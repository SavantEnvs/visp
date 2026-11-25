// Wireframe-simulator scene files (.bnd): vpWireFrameSimulator::initScene(const char *), which
// runs the old C lexer/parser in modules/robot/src/wireframe-simulator (vpLex, vpParser, vpBoundio).
//
// Every input goes to the parser unfiltered. Known upstream bugs it reaches (expected defects):
// lex()/lexecho() index the 256+2-entry character tables with a signed char (`chtbl[(int)CURC]`,
// vpLex.cpp:272 and every CURC lookup), so any byte >= 0x80 reads up to 128 bytes before the
// table; and lex() accumulates integer literals in an `int` (`myint = myint * 10 + digit`,
// vpLex.cpp:341), a signed overflow on a run of 10+ digits.
//
// State kept on purpose: when an input fails to parse, the camera and partial scene that
// vpWireFrameSimulator parsed into its private members are not freed (memory only, no fd or
// global parser state), so long runs can see an occasional out-of-memory.
//
// State reset: on a syntax error lexerr() throws out of set_scene() (vpScene.cpp) before its
// close_source()/close_lex()/close_keyword()/fclose() run, and before the parser pops the error
// messages it pushed. In a one-shot program that is harmless; in a fuzz loop it leaks ~270 KB and a
// file descriptor per bad input (OOM / EMFILE after a few thousand inputs) and fills the 32-entry
// error stack, after which every input fails in pusherr(). After a throw the harness runs the same
// cleanup set_scene() would have run, so every input starts from the state a fresh process has.
#include "fuzz_common.h"
#include <climits>
#include <fcntl.h>
#include <visp3/robot/vpWireFrameSimulator.h>

BEGIN_VISP_NAMESPACE
void close_source(void);
void close_lex(void);
void close_keyword(void);
void poperr(void);
END_VISP_NAMESPACE

// Closes every descriptor that still refers to `path` (the FILE* set_scene() leaked).
static void close_leaked_fds(const std::string &path)
{
  for (int fd = 3; fd < 1024; ++fd) {
    if (fcntl(fd, F_GETFD) == -1)
      continue;
    char link[64], target[PATH_MAX];
    snprintf(link, sizeof(link), "/proc/self/fd/%d", fd);
    ssize_t n = readlink(link, target, sizeof(target) - 1);
    if (n <= 0)
      continue;
    target[n] = '\0';
    if (path == target)
      close(fd);
  }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  std::string path = fuzz_write_tmp(data, size, ".bnd");
  bool parse_error = false;
  try {
    vpWireFrameSimulator sim;
    try {
      sim.initScene(path.c_str());
    }
    catch (const vpException &) {
      parse_error = true;
    }
  }
  catch (const vpException &) {
  }
  catch (const std::exception &) {
  }
  if (parse_error) {
    close_source();
    close_lex();
    close_keyword();
    try {
      for (int i = 0; i < 64; ++i)
        poperr(); // throws once the error stack is empty
    }
    catch (const vpException &) {
    }
    close_leaked_fds(path);
  }
  unlink(path.c_str());
  return 0;
}
