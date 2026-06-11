#include "Html.h"

#include <sstream>

namespace news_server {

std::string escapeHtml(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&#39;";  break;
            default:   out += c;
        }
    }
    return out;
}

std::string nl2br(const std::string& escaped) {
    std::string out;
    out.reserve(escaped.size());
    for (std::size_t i = 0; i < escaped.size(); ++i) {
        if (escaped[i] == '\n') out += "<br/>";
        else if (escaped[i] == '\r') {/* skip */}
        else out += escaped[i];
    }
    return out;
}

std::string renderLayout(const std::string& title,
                         const std::string& body,
                         const std::string& currentUser) {
    std::ostringstream os;
    os << "<!DOCTYPE html>\n"
       << "<html lang=\"uk\"><head>\n"
       << "<meta charset=\"UTF-8\"/>\n"
       << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"/>\n"
       << "<title>" << escapeHtml(title) << " · Онлайн-новини</title>\n"
       << "<style>\n"
       << "  :root { --fg:#1a1a1a; --muted:#666; --bg:#fafafa; --card:#fff;\n"
       << "          --accent:#1d4ed8; --border:#e5e7eb; --danger:#dc2626; }\n"
       << "  * { box-sizing: border-box; }\n"
       << "  body { margin:0; font-family: -apple-system, BlinkMacSystemFont,\n"
       << "         'Segoe UI', sans-serif; background:var(--bg); color:var(--fg); }\n"
       << "  header { background:var(--card); border-bottom:1px solid var(--border);\n"
       << "           padding: 1rem 2rem; display:flex; align-items:center; gap:2rem; }\n"
       << "  header h1 { margin:0; font-size:1.25rem; }\n"
       << "  header h1 a { color:var(--fg); text-decoration:none; }\n"
       << "  header nav { display:flex; gap:1rem; flex:1; }\n"
       << "  header nav a { color:var(--muted); text-decoration:none; font-size:0.95rem; }\n"
       << "  header nav a:hover { color:var(--accent); }\n"
       << "  header .user { color:var(--muted); font-size:0.9rem; }\n"
       << "  header .user a { color:var(--accent); text-decoration:none; }\n"
       << "  main { max-width: 56rem; margin: 2rem auto; padding: 0 1.5rem; }\n"
       << "  .card { background:var(--card); border:1px solid var(--border);\n"
       << "          border-radius:8px; padding:1.25rem 1.5rem; margin-bottom:1rem; }\n"
       << "  .card h2 { margin-top:0; }\n"
       << "  .card h2 a { color:var(--fg); text-decoration:none; }\n"
       << "  .card h2 a:hover { color:var(--accent); }\n"
       << "  .meta { color:var(--muted); font-size:0.85rem; margin-bottom:0.5rem; }\n"
       << "  .meta a { color:var(--muted); }\n"
       << "  .tags { margin-top:0.75rem; }\n"
       << "  .tags a { display:inline-block; background:#eef2ff; color:#3730a3;\n"
       << "            padding:0.15rem 0.5rem; border-radius:4px;\n"
       << "            text-decoration:none; font-size:0.8rem; margin-right:0.25rem; }\n"
       << "  form { display:flex; flex-direction:column; gap:0.75rem; }\n"
       << "  label { display:flex; flex-direction:column; gap:0.25rem; font-size:0.9rem; color:var(--muted); }\n"
       << "  input, textarea, select {\n"
       << "    font:inherit; padding:0.5rem 0.65rem; border:1px solid var(--border);\n"
       << "    border-radius:6px; background:white;\n"
       << "  }\n"
       << "  textarea { min-height:8rem; resize:vertical; }\n"
       << "  button, .btn {\n"
       << "    background:var(--accent); color:white; border:none; padding:0.6rem 1.2rem;\n"
       << "    border-radius:6px; font-size:0.95rem; cursor:pointer; text-decoration:none;\n"
       << "    display:inline-block;\n"
       << "  }\n"
       << "  .btn-danger { background:var(--danger); }\n"
       << "  .btn-ghost { background:transparent; color:var(--fg); border:1px solid var(--border); }\n"
       << "  .row { display:flex; gap:1rem; }\n"
       << "  .row > * { flex:1; }\n"
       << "  .alert { background:#fef2f2; border:1px solid #fecaca; color:#991b1b;\n"
       << "           padding:0.75rem; border-radius:6px; margin-bottom:1rem; }\n"
       << "  .filter-bar { background:var(--card); border:1px solid var(--border);\n"
       << "                border-radius:8px; padding:1rem 1.25rem; margin-bottom:1.5rem; }\n"
       << "  .filter-bar .row { flex-wrap:wrap; }\n"
       << "  .empty { color:var(--muted); text-align:center; padding:2rem; }\n"
       << "  article p { line-height:1.6; }\n"
       << "</style></head><body>\n"
       << "<header>\n"
       << "  <h1><a href=\"/\">Онлайн-новини</a></h1>\n"
       << "  <nav>\n"
       << "    <a href=\"/\">Всі новини</a>\n"
       << "    <a href=\"/categories\">Рубрики</a>\n"
       << "    <a href=\"/tags\">Теги</a>\n"
       << "    <a href=\"/authors\">Автори</a>\n"
       << "  </nav>\n"
       << "  <div class=\"user\">";

    if (!currentUser.empty()) {
        os << escapeHtml(currentUser)
           << " · <a href=\"/news/new\">+ Нова новина</a>"
           << " · <form method=\"post\" action=\"/logout\" style=\"display:inline\">"
              "<button class=\"btn-ghost\" style=\"padding:0.2rem 0.5rem;font-size:0.85rem\">Вийти</button></form>";
    } else {
        os << "<a href=\"/login\">Вхід</a> · <a href=\"/register\">Реєстрація</a>";
    }
    os << "</div></header>\n<main>\n" << body << "\n</main></body></html>";
    return os.str();
}

} // namespace news_server
