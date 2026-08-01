#pragma once

#include <string>
#include <regex>

/**
 * ExpressionFormat - stub tanpa rift dependency.
 * Fitur dynamic expression {colorRange(...)} dll. di-disable,
 * format string di-pass as-is (tanpa evaluasi ekspresi).
 * Rift library (EclipseMenu/rift) private dan tidak bisa diakses di CI.
 */
namespace ExpressionFormat {

    inline void initialize() {
        // no-op: rift disabled
    }

    // Return true kalau ada {expr} di string
    inline bool hasExpressions(const std::string& text) {
        return text.find('{') != std::string::npos &&
               text.find('}') != std::string::npos;
    }

    // Strip semua {expr} dari string, kembalikan teks biasa
    inline std::string format(const std::string& text,
                              double /*numericValue*/ = 0.0,
                              const std::string& /*stringValue*/ = "") {
        // Hapus semua {expr} — fallback ke plain text tanpa color tags
        std::string result;
        result.reserve(text.size());
        bool inExpr = false;
        for (char c : text) {
            if (c == '{') { inExpr = true;  continue; }
            if (c == '}') { inExpr = false; continue; }
            if (!inExpr) result += c;
        }
        return result;
    }

} // namespace ExpressionFormat
