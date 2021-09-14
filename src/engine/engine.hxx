#include <cstdint>
#include <string>
#include <vector>
#include <exception>
#include <unordered_map>

#include "../def/warnings.dxx"
#include "../../lib/chunk.hxx"
#include "../../lib/buffer.hxx"
#include "../../lib/filter.hxx"

// TODO: Remove these
#include <unordered_set>

#include "bridge.hxx"

using std::string;

#define ERYN_INTERNAL_EXCEPTION(msg) (InternalException(msg, __FILE__, __LINE__))

namespace Eryn {
enum class EngineMode {
    NORMAL,  // Normal speed: the engine is not limited in any way.
    STRICT   // Full speed: the engine is limited to basic content inside the templates.
};

struct Options {
    struct {
        bool bypassCache            : 1;
        bool throwOnEmptyContent    : 1;
        bool throwOnMissingEntry    : 1;
        bool throwOnCompileDirError : 1;
        bool ignoreBlankPlaintext   : 1;
        bool logRenderTime          : 1;
        bool cloneIterators         : 1;
        bool cloneBackups           : 1;
        bool cloneLocalInLoops      : 1;
        bool debugDumpOSH           : 1;
    } flags;

    EngineMode mode;
    string workingDir;

    struct {
        char escape;

        string start;
        string end;
        string bodyEnd;
        string voidStart;
        string commentStart;

        string commentEnd;
        string conditionalStart;
        string elseStart;
        string elseConditionalStart;

        string loopStart;
        string loopSeparator;
        string loopReverse;
        string componentStart;
        string componentSeparator;
        string componentSelf;
    } templates;

    Options();
};

class Cache {
    std::unordered_map<string, ConstBuffer> entries;

  public:
    ~Cache();

    void         add(const string& key, ConstBuffer&& value);
    ConstBuffer& get(const string& key);
    bool         has(const string& key) const;
};

class Engine {
  public:
    Options opts;
    Cache   cache;

    void compile(const char* path);
    void compile_string(const char* alias, const char* str);
    void compile_dir(const char* path, std::vector<string> filters);

    ConstBuffer render(Bridge& bridge, const char* path);
    ConstBuffer render_string(Bridge& bridge, const char* alias);

  private:
    void        compile_dir(const char* path, const char* rel, const FilterInfo& info);
    ConstBuffer compile_file(const char* path);
    ConstBuffer compile_bytes(ConstBuffer& inputBuffer, const char* wd, const char* path = "");
};

class InternalException : public std::exception {
  public:
    string message;
    string function;
    int    line;

    InternalException(string msg, string fn, int ln);

    const char* what() const noexcept override;
};

class CompilationException : public std::exception {
  public:
      string file;
      string msg;
      string description;
      Chunk chunk;

      string message;

      CompilationException(const char* file, const char* msg, const char* description);
      CompilationException(const char* file, const char* msg, const char* description, const Chunk& chunk);

      const char* what() const noexcept override;
};

class RenderingException : public std::exception {
  public:
      string msg;
      string description;
      string path;
      ConstBuffer token;

      string message;

      RenderingException(const char* msg, const char* description);
      RenderingException(const char* msg, const char* description, const char* path);
      RenderingException(const char* msg, const char* description, ConstBuffer token);
      RenderingException(const char* msg, const char* description, const char* path, ConstBuffer token);

      const char* what() const noexcept override;
};
}