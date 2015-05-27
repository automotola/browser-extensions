#include "stdafx.h"
#include "ScriptExtensions.h"
#include "vendor.h"

/**
 * Construction
 *
 * @path Location of script extension directory
 */
ScriptExtensions::ScriptExtensions(const bfs::wpath& path, bool reload) : m_path(path), pathManifest(path / L"manifest.json")
{
  logger->debug(L"ScriptExtensions::ScriptExtensions -> " + path.wstring() + L" -> " + boost::lexical_cast<wstring>(reload));

  if (reload)
    Reload();
}

/**
 * Reload script cache from filesystem
 */
void ScriptExtensions::Reload()
{
  // read manifest.json
  manifest = ParseManifest(pathManifest);
  if (!manifest) {
    logger->debug(L"ScriptExtensions::Reload could not load manifest: " + pathManifest.wstring());
    // TODO user-visible error please
    // TODO exit BHO cleanly
    return;
  }
  manifest->dump();

  // cache background_page
  bfs::wpath path = m_path / manifest->background_page;
  if (!bfs::exists(path)) {
    logger->error(L"ScriptExtensions::Reload background_page not found: " + path.wstring());
  }

  std::wifstream stream(path.wstring());
  background_page = wstringpointer(new wstring((std::istreambuf_iterator<wchar_t>(stream)), (std::istreambuf_iterator<wchar_t>())));

  // cache content_scripts & styles
  for (auto& script : manifest->content_scripts) {
    for (auto& name : script.js) {
      bfs::wpath path = m_path / name;
      if (!bfs::exists(path)) {
        logger->error(L"ScriptExtensions::Reload content_script not found: " + path.wstring());
        m_scripts[name] = wstringpointer();
        ::MessageBox(0, wstring(L"You have an invalid script specified in your config.json: " + path.wstring()).c_str(), VENDOR_COMPANY_NAME, MB_TASKMODAL | MB_ICONEXCLAMATION);
        continue;
      }
      std::wifstream stream(path.wstring());
      wstringpointer script(new wstring((std::istreambuf_iterator<wchar_t>(stream)), (std::istreambuf_iterator<wchar_t>())));
      m_scripts[name] = script;
      logger->info(L"ScriptExtension::Reload cached content_script js: " + path.wstring());
    }

    for (auto& name : script.css) {
      bfs::wpath path = m_path / name;
      if (!bfs::exists(path)) {
        logger->error(L"ScriptExtensions::Reload content_script not found: " + path.wstring());
        ::MessageBox(NULL, wstring(L"You have an invalid stylesheet specified in your config.json: " + path.wstring()).c_str(), VENDOR_COMPANY_NAME, MB_TASKMODAL | MB_ICONEXCLAMATION);
        m_styles[name] = wstringpointer();
        continue;
      }
      std::wifstream stream(path.wstring());
      wstringpointer style(new wstring((std::istreambuf_iterator<wchar_t>(stream)),
        (std::istreambuf_iterator<wchar_t>())));
      m_styles[name] = style;
      logger->info(L"ScriptExtension::Reload cached content_script css: " + path.wstring());
    }
  }
}

/**
 * Parse extension manifest.json 
 */
Manifest::pointer ScriptExtensions::ParseManifest(const bfs::wpath& path)
{
    logger->info(L"ScriptExtensions::ParseManifest loading: " + path.wstring());

    std::wifstream stream(path.wstring());
    wstringpointer content(new wstring((std::istreambuf_iterator<wchar_t>(stream)),
                                       (std::istreambuf_iterator<wchar_t>())));
    if (content->empty()) {
        return Manifest::pointer();
    } 
    // logger->debug(L"manifest: " + *content);

    json_spirit::wValue v;
    json_spirit::read(*content, v);
    json_spirit::wObject json = v.get_obj();

    // parse manifest.json
    //logger->debug(L"parsed manifest: " + json_spirit::write(v));
    Manifest lmanifest = {
        json_util::wfind_str(json, L"name"),
        json_util::wfind_str(json, L"author"),
        json_util::wfind_str(json, L"version"),
        json_util::wfind_str(json, L"description"),
        json_util::wfind_str(json, L"uuid"),
        json_util::wfind_str(json, L"background_page")
    };
    json_spirit::wArray permissions = json_util::wfind_array(json, L"permissions");
    lmanifest.permissions = json_util::wstrings_to_vector(permissions);

    json_spirit::wArray content_scripts = json_util::wfind_array(json, L"content_scripts");
    for (auto& i : content_scripts) {
        json_spirit::wObject content_script = i.get_obj();
        json_spirit::wArray matches = json_util::wfind_array(content_script, L"matches");
        Manifest::ContentScript _content_script = {
            json_util::wfind_strarray(content_script, L"matches"),
            json_util::wfind_strarray(content_script, L"exclude_matches"),
            json_util::wfind_strarray(content_script, L"js"),
            json_util::wfind_strarray(content_script, L"css"),
            json_util::wfind_str(content_script, L"run_at"),
            json_util::wfind_bool(content_script, L"all_frames"),
        };
       lmanifest.content_scripts.push_back(_content_script);
    }

    if (lmanifest.content_scripts.empty()) {
        logger->debug(L"No activations specified. Adding default activation");
        wstringvector matches;
        matches.push_back(L"file://*");
        matches.push_back(L"http://*");
        matches.push_back(L"https://*");
        wstringvector js;
        js.push_back(L"forge/all.js");
        Manifest::ContentScript _content_script = {
            matches,
			wstringvector(),
            js,
            wstringvector(),
            L"start",
            false
        };
        lmanifest.content_scripts.push_back(_content_script);
    }

    json_spirit::wObject browser_action = json_util::wfind_obj(json, L"browser_action");
    Manifest::BrowserAction _browser_action = { 
        json_util::wfind_str(browser_action, L"default_popup"),
        json_util::wfind_str(browser_action, L"default_title"),
        json_util::wfind_str(browser_action, L"default_icon"),
        json_util::wfind_strmap(browser_action, L"default_icons")
    };
    lmanifest.browser_action = _browser_action;
    json_spirit::wObject logging = json_util::wfind_obj(json, L"logging");
    Manifest::Logging _logging = { 
        json_util::wfind_str(logging, L"level"),
        json_util::wfind_bool(logging, L"console"),
        json_util::wfind_str(logging, L"filename")
    };
    lmanifest.logging = _logging;

    return Manifest::pointer(new Manifest(lmanifest));
}
