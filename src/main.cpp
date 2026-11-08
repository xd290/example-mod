#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/async.hpp>
#include <sstream>
#include <string>

using namespace geode::prelude;

static const char* SISTEMA =
    "Eres un generador de niveles para Geometry Dash. "
    "Responde SOLO con lineas de texto, una por objeto, con el formato: id x y. "
    "Sin explicaciones, sin comillas, sin markdown. "
    "x es la columna (0 a 70) e y es la fila (0 es el suelo, 1 es encima, maximo 6). "
    "Ids permitidos: 1 = bloque, 8 = pico, 39 = pico chico, "
    "35 = pad amarillo (salto alto), 36 = orbe amarillo (salto en el aire). "
    "No pongas bloques como piso porque el suelo ya existe. "
    "Los picos y pads sobre el suelo van en y=0. Un pico sobre un bloque va una fila arriba del bloque. "
    "El nivel debe poder pasarse con un cubo: maximo 3 picos seguidos y al menos 4 columnas libres entre obstaculos. "
    "Ordena las lineas de izquierda a derecha.";

static std::string escapar(std::string const& s) {
    std::string o;
    for (unsigned char c : s) {
        switch (c) {
            case '"': o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n"; break;
            case '\r': o += "\\r"; break;
            case '\t': o += "\\t"; break;
            default:
                if (c < 0x20) o += ' ';
                else o += static_cast<char>(c);
        }
    }
    return o;
}

static std::string armarCuerpo(std::string const& modelo, std::string const& prompt, int maximo) {
    std::string sistema = std::string(SISTEMA) + " Usa como maximo " + std::to_string(maximo) + " lineas.";
    return "{\"model\":\"" + escapar(modelo) + "\","
           "\"max_tokens\":2000,"
           "\"system\":\"" + escapar(sistema) + "\","
           "\"messages\":[{\"role\":\"user\",\"content\":\"" + escapar(prompt) + "\"}]}";
}

// Saca el texto de la respuesta de la API
static std::string extraerTexto(std::string const& body) {
    auto pos = body.find("\"text\"");
    if (pos == std::string::npos) return "";
    pos = body.find('"', pos + 6);
    if (pos == std::string::npos) return "";
    std::string out;
    for (size_t i = pos + 1; i < body.size(); i++) {
        char c = body[i];
        if (c == '"') break;
        if (c == '\\' && i + 1 < body.size()) {
            char n = body[++i];
            if (n == 'n') out += '\n';
            else if (n == 't') out += ' ';
            else if (n == 'r') {}
            else if (n == 'u') i += 4;
            else out += n;
        } else {
            out += c;
        }
    }
    return out;
}

static int colocarObjetos(std::string const& texto, int maximo) {
    auto editor = LevelEditorLayer::get();
    if (!editor) return 0;
    int creados = 0;
    std::istringstream lineas(texto);
    std::string linea;
    while (std::getline(lineas, linea) && creados < maximo) {
        std::istringstream ss(linea);
        int id;
        float x, y;
        if (!(ss >> id >> x >> y)) continue;
        if (id != 1 && id != 8 && id != 39 && id != 35 && id != 36) continue;
        CCPoint pos = { 300.f + 15.f + x * 30.f, 105.f + y * 30.f };
        editor->createObject(id, pos, false);
        creados++;
    }
    return creados;
}

// ---------- Popup para la API key ----------
class ClavePopup : public Popup {
protected:
    TextInput* m_input = nullptr;

    bool init() {
        if (!Popup::init(320.f, 160.f)) return false;
        this->setTitle("API Key");
        float cx = m_size.width / 2;

        m_input = TextInput::create(280.f, "Pega tu API key", "chatFont.fnt");
        m_input->setPosition({ cx, 95.f });
        m_mainLayer->addChild(m_input);

        auto menu = CCMenu::create();
        menu->setPosition({ 0.f, 0.f });
        m_mainLayer->addChild(menu);

        auto spr = ButtonSprite::create("Guardar");
        spr->setScale(0.8f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(ClavePopup::onGuardar));
        btn->setPosition({ cx, 40.f });
        menu->addChild(btn);
        return true;
    }

    void onGuardar(CCObject*) {
        std::string k = std::string(m_input->getString());
        Mod::get()->setSettingValue<std::string>("api-key", k);
        Notification::create("API key guardada", NotificationIcon::Success, 1.5f)->show();
        this->keyBackClicked();
    }

public:
    static ClavePopup* create() {
        auto ret = new ClavePopup();
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

// ---------- Popup principal de la IA ----------
class IAPopup : public Popup {
protected:
    TextInput* m_input = nullptr;
    CCLabelBMFont* m_estado = nullptr;
    bool m_ocupado = false;
    async::TaskHolder<web::WebResponse> m_listener;

    bool init() {
        if (!Popup::init(340.f, 230.f)) return false;
        this->setTitle("Generador de niveles IA");
        float cx = m_size.width / 2;

        m_input = TextInput::create(300.f, "Describe tu nivel...", "chatFont.fnt");
        m_input->setPosition({ cx, 160.f });
        m_mainLayer->addChild(m_input);

        m_estado = CCLabelBMFont::create("", "chatFont.fnt");
        m_estado->setPosition({ cx, 120.f });
        m_estado->setScale(0.6f);
        m_mainLayer->addChild(m_estado);

        auto menu = CCMenu::create();
        menu->setPosition({ 0.f, 0.f });
        m_mainLayer->addChild(menu);

        auto s1 = ButtonSprite::create("Generar");
        s1->setScale(0.8f);
        auto b1 = CCMenuItemSpriteExtra::create(s1, this, menu_selector(IAPopup::onGenerar));
        b1->setPosition({ cx, 80.f });
        menu->addChild(b1);

        auto s2 = ButtonSprite::create("API Key");
        s2->setScale(0.7f);
        auto b2 = CCMenuItemSpriteExtra::create(s2, this, menu_selector(IAPopup::onClave));
        b2->setPosition({ cx - 80.f, 35.f });
        menu->addChild(b2);

        auto s3 = ButtonSprite::create("Ajustes");
        s3->setScale(0.7f);
        auto b3 = CCMenuItemSpriteExtra::create(s3, this, menu_selector(IAPopup::onAjustes));
        b3->setPosition({ cx + 80.f, 35.f });
        menu->addChild(b3);
        return true;
    }

    void onClave(CCObject*) {
        ClavePopup::create()->show();
    }

    void onAjustes(CCObject*) {
        openSettingsPopup(Mod::get());
    }

    void onGenerar(CCObject*) {
        if (m_ocupado) return;
        std::string prompt = std::string(m_input->getString());
        std::string clave = Mod::get()->getSettingValue<std::string>("api-key");
        if (clave.empty()) {
            m_estado->setString("Falta la API key (boton API Key)");
            return;
        }
        if (prompt.empty()) {
            m_estado->setString("Escribe que nivel quieres");
            return;
        }
        std::string modelo = Mod::get()->getSettingValue<std::string>("model");
        int maximo = static_cast<int>(Mod::get()->getSettingValue<int64_t>("max-objects"));

        m_ocupado = true;
        m_estado->setString("Pensando... puede tardar");

        auto req = web::WebRequest();
        req.header("x-api-key", clave);
        req.header("anthropic-version", "2023-06-01");
        req.header("content-type", "application/json");
        req.timeout(std::chrono::seconds(90));
        req.bodyString(armarCuerpo(modelo, prompt, maximo));

        m_listener.spawn(
            req.post("https://api.anthropic.com/v1/messages"),
            [this, maximo](web::WebResponse res) {
                m_ocupado = false;
                if (!res.ok()) {
                    int code = res.code();
                    std::string msg = "Error " + std::to_string(code);
                    if (code == 401) msg = "API key invalida (401)";
                    else if (code == 429) msg = "Limite o saldo agotado (429)";
                    else if (code == 404) msg = "Modelo no encontrado (404)";
                    m_estado->setString(msg.c_str());
                    return;
                }
                std::string body = res.string().unwrapOr("");
                std::string texto = extraerTexto(body);
                int n = colocarObjetos(texto, maximo);
                std::string msg = "Listo: " + std::to_string(n) + " objetos";
                if (n == 0) msg = "La IA no devolvio objetos validos";
                m_estado->setString(msg.c_str());
            }
        );
    }

public:
    static IAPopup* create() {
        auto ret = new IAPopup();
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

// ---------- Botón en el editor ----------
class $modify(IAEditorUI, EditorUI) {
    bool init(LevelEditorLayer* lel) {
        if (!EditorUI::init(lel)) return false;

        auto menu = CCMenu::create();
        auto spr = ButtonSprite::create("IA");
        spr->setScale(0.8f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(IAEditorUI::onIA));
        menu->addChild(btn);

        auto win = CCDirector::sharedDirector()->getWinSize();
        menu->setPosition({ 45.f, win.height - 110.f });
        this->addChild(menu, 100);
        return true;
    }

    void onIA(CCObject*) {
        IAPopup::create()->show();
    }
};
