#include <cstdint>
#include <string>
#include <vector>
#include <exception>
#include <unordered_map>

#include "../def/warnings.dxx"
#include "../../lib/chunk.hxx"
#include "../../lib/buffer.hxx"
#include "../../lib/filter.hxx"

using std::string;

#define ERYN_INTERNAL_EXCEPTION(msg) (InternalException(msg, __FILE__, __LINE__))

namespace Eryn {
struct Options {
    struct {
        bool bypassCache            : 1;
        bool throwOnEmptyContent    : 1;
        bool throwOnMissingEntry    : 1;
        bool throwOnCompileDirError : 1;
        bool ignoreBlankPlaintext   : 1;
        bool logRenderTime          : 1;
        bool cloneIterators         : 1;
        bool debugDumpOSH           : 1;
    } flags;

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
    void         add(string key, ConstBuffer&& value);
    ConstBuffer& get(string key);
    bool         has(string key) const;
};

class Engine {
  public:
    Options opts;
    Cache   cache;

    Engine();

    void compile(const char* path);
    void compileString(const char* alias, const char* str);
    void compileDir(const char* path, std::vector<string> filters);

  private:
    void   compileDir(const char* path, const char* rel, const FilterInfo& info);
    ConstBuffer compileFile(const char* path);
    ConstBuffer compileBytes(ConstBuffer& inputBuffer, const char* wd, const char* path = "");
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
}