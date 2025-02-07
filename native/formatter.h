#include <format>
#include <vector>
#include <filesystem>

template <typename T>
struct std::formatter<std::vector<T>> : std::formatter<T> {
    auto format(const std::vector<T>& vec, std::format_context& ctx) const {
        auto out = ctx.out();
        out = std::format_to(out, "{{");
        for (size_t i = 0; i < vec.size(); ++i) {
            if (i > 0) out = std::format_to(out, ", ");
            out = std::formatter<T>::format(vec[i], ctx);
        }
        return std::format_to(out, "}}");
    }
};

template <>
struct std::formatter<std::filesystem::path> : std::formatter<std::string> {
    auto format(const std::filesystem::path& path, std::format_context& ctx) const {
        return std::formatter<std::string>::format(path.string(), ctx);
    }
};
