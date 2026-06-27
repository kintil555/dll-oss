#pragma once

#include "ScriptLib.hpp"
#include <curl/curl/curl.h>
#include <thread>

// Forward-declare helpers implemented as static member functions to avoid
// deeply-nested lambda instantiation that triggers MSVC 19.51 ICE (C1001).
class NetworkLib : public ScriptLib {
public:
    void initialize(lua_State* state) override {
        using namespace luabridge;

        getGlobalNamespace(state)
            .beginNamespace("network")
                .addFunction("get",       &NetworkLib::netGet)
                .addFunction("post",      &NetworkLib::netPost)
                .addFunction("getAsync",  &NetworkLib::netGetAsync)
                .addFunction("postAsync", &NetworkLib::netPostAsync)
            .endNamespace();
    }

private:
    // ── sync helpers ────────────────────────────────────────────────────────

    static std::string netGet(const std::string& url) {
        std::string response;
        CURL* curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            curl_easy_perform(curl);
            curl_easy_cleanup(curl);
        }
        return response;
    }

    static std::string netPost(const std::string& url, const std::string& data) {
        std::string response;
        CURL* curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            curl_easy_perform(curl);
            curl_easy_cleanup(curl);
        }
        return response;
    }

    // ── async helpers ────────────────────────────────────────────────────────

    static void netGetAsync(const std::string& url, luabridge::LuaRef callback, lua_State* L) {
        if (!callback.isFunction()) {
            luaL_error(L, "network.getAsync: second argument must be a callback function");
            return;
        }
        auto* script = getScriptFromState(L);
        if (!script) return;

        callback.push(L);
        int callbackRef = luaL_ref(L, LUA_REGISTRYINDEX);
        auto weakScript = script->weak_from_this();

        std::thread([url, callbackRef, weakScript]() {
            doGetRequest(url, callbackRef, weakScript, "network.getAsync");
        }).detach();
    }

    static void netPostAsync(const std::string& url, const std::string& data,
                             luabridge::LuaRef callback, lua_State* L) {
        if (!callback.isFunction()) {
            luaL_error(L, "network.postAsync: third argument must be a callback function");
            return;
        }
        auto* script = getScriptFromState(L);
        if (!script) return;

        callback.push(L);
        int callbackRef = luaL_ref(L, LUA_REGISTRYINDEX);
        auto weakScript = script->weak_from_this();

        std::thread([url, data, callbackRef, weakScript]() {
            doPostRequest(url, data, callbackRef, weakScript, "network.postAsync");
        }).detach();
    }

    // ── curl workers (called from detached threads) ──────────────────────────

    struct RequestResult {
        std::string response;
        long        statusCode = 0;
        bool        success    = false;
    };

    static void doGetRequest(const std::string& url, int callbackRef,
                             std::weak_ptr<Script> weakScript,
                             const char* logTag) {
        RequestResult r;
        CURL* curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &r.response);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
            r.success = (curl_easy_perform(curl) == CURLE_OK);
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &r.statusCode);
            curl_easy_cleanup(curl);
        }
        fireCallback(callbackRef, std::move(r), weakScript, logTag);
    }

    static void doPostRequest(const std::string& url, const std::string& data,
                              int callbackRef,
                              std::weak_ptr<Script> weakScript,
                              const char* logTag) {
        RequestResult r;
        CURL* curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &r.response);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
            r.success = (curl_easy_perform(curl) == CURLE_OK);
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &r.statusCode);
            curl_easy_cleanup(curl);
        }
        fireCallback(callbackRef, std::move(r), weakScript, logTag);
    }

    static void fireCallback(int callbackRef, RequestResult r,
                             std::weak_ptr<Script> weakScript,
                             const char* logTag) {
        if (auto script = weakScript.lock()) {
            // Capture by value — no nested lambdas passed to LuaBridge
            std::string resp    = std::move(r.response);
            long        status  = r.statusCode;
            bool        success = r.success;
            std::string tag     = logTag;

            script->queueCallback([callbackRef, resp, status, success, tag](lua_State* L) {
                lua_rawgeti(L, LUA_REGISTRYINDEX, callbackRef);
                if (lua_isfunction(L, -1)) {
                    lua_pushstring(L, resp.c_str());
                    lua_pushinteger(L, status);
                    lua_pushboolean(L, success);
                    if (lua_pcall(L, 3, 0, 0) != LUA_OK) {
                        std::string err = lua_tostring(L, -1);
                        Logger::script(true, "{} callback error: {}", tag, err);
                        lua_pop(L, 1);
                    }
                } else {
                    lua_pop(L, 1);
                }
                luaL_unref(L, LUA_REGISTRYINDEX, callbackRef);
            });
        }
    }

    // ── utilities ─────────────────────────────────────────────────────────────

    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
        output->append(static_cast<char*>(contents), size * nmemb);
        return size * nmemb;
    }

    static Script* getScriptFromState(lua_State* L) {
        lua_getglobal(L, "_script_instance");
        if (!lua_islightuserdata(L, -1)) {
            lua_pop(L, 1);
            luaL_error(L, "Script instance not found");
            return nullptr;
        }
        auto* script = static_cast<Script*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        return script;
    }
};
