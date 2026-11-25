// Matrix file readers: vpArray2D<double>::load (text and binary) and vpArray2D<double>::loadYAML,
// with and without the header out-buffer. [0] selects the reader; the rest is the file.
//
// The only input the harness skips is a matrix whose declared size the file cannot back: the
// readers resize to rows x cols straight from the header (up to 4e9 x 4e9) before reading a single
// element, so "4000000000 4000000000" is an out-of-memory, not a parsing bug. The harness reads the
// dimensions the way each reader does and rejects them only when both are known and rows, cols or
// rows*cols exceed what the bytes present could hold (one byte per text/YAML element, sizeof(double)
// per binary element). Every other malformed file goes to the reader as is, including the upstream
// bugs the readers have on bad input: rows/cols read uninitialized when the stream fails before
// `>> rows` (a comment line of FILENAME_MAX bytes or more, or a file of comments only), the binary
// header loop that never ends without a NUL, and a binary file too short for the two dimensions.
#include "fuzz_common.h"
#include <cstdio>
#include <cstring>
#include <sstream>
#include <visp3/core/vpArray2D.h>

static char header[4096];

// rows/cols as the reader will see them; *_known is false when the reader leaves them
// uninitialized (then the size is not checked: that is the upstream bug, not a huge matrix).
struct Dims
{
  bool rows_known = false, cols_known = false;
  uint64_t rows = 0, cols = 0;
};

static bool dims_fit(const Dims &d, uint64_t budget)
{
  if (!d.rows_known || !d.cols_known)
    return true;
  return d.rows <= budget && d.cols <= budget && d.rows * d.cols <= budget;
}

// load(binary=false): the same header loop as the reader (getline into a FILENAME_MAX buffer,
// skip "# " lines, rewind), then `>> rows >> cols`, on a copy of the input.
static Dims text_dims(const std::string &s)
{
  Dims d;
  std::istringstream file(s);
  bool headerIsDecoded = false;
  do {
    std::streampos pos = file.tellg();
    char line[FILENAME_MAX];
    file.getline(line, FILENAME_MAX);
    std::string line_(line);
    if (line_.compare(0, 2, "# ") != 0) {
      file.seekg(pos, file.beg);
      headerIsDecoded = true;
    }
  } while (!headerIsDecoded);
  unsigned int rows = 0, cols = 0;
  if (!file.good())
    return d; // the sentry fails: the reader's rows/cols stay uninitialized
  file >> rows;
  d.rows_known = true;
  d.rows = rows;
  if (!file.good())
    return d;
  file >> cols;
  d.cols_known = true;
  d.cols = cols;
  return d;
}

// load(binary=true): header up to the first NUL, then two raw unsigned ints.
static Dims binary_dims(const uint8_t *p, size_t n)
{
  Dims d;
  const void *nul = memchr(p, 0, n);
  if (!nul)
    return d;
  size_t off = static_cast<const uint8_t *>(nul) - p + 1;
  if (n - off < 2 * sizeof(unsigned int))
    return d;
  unsigned int rows, cols;
  memcpy(&rows, p + off, sizeof rows);
  memcpy(&cols, p + off + sizeof rows, sizeof cols);
  d.rows_known = d.cols_known = true;
  d.rows = rows;
  d.cols = cols;
  return d;
}

// loadYAML: first "rows:" / "cols:" header lines before "data:" (both start at 0).
static Dims yaml_dims(const std::string &s)
{
  Dims d;
  d.rows_known = d.cols_known = true;
  std::istringstream in(s);
  std::string line, subs;
  unsigned int rows = 0, cols = 0;
  while (std::getline(in, line)) {
    if (rows == 0 && line.compare(0, 5, "rows:") == 0) {
      std::stringstream ss(line);
      ss >> subs;
      ss >> rows;
    }
    else if (cols == 0 && line.compare(0, 5, "cols:") == 0) {
      std::stringstream ss(line);
      ss >> subs;
      ss >> cols;
    }
    else if (line.compare(0, 5, "data:") == 0) {
      break;
    }
  }
  d.rows = rows;
  d.cols = cols;
  return d;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  if (size < 1)
    return -1;
  const unsigned mode = data[0] % 6;
  const uint8_t *file = data + 1;
  const size_t n = size - 1;
  const std::string s(reinterpret_cast<const char *>(file), n);

  bool ok;
  switch (mode / 2) {
  case 0: ok = dims_fit(text_dims(s), n); break;
  case 1: ok = dims_fit(binary_dims(file, n), n / sizeof(double)); break;
  default: ok = dims_fit(yaml_dims(s), n); break;
  }
  if (!ok)
    return -1;

  // The header can never be longer than the file, so this buffer always fits it.
  char *hdr = ((mode & 1) && n < sizeof(header)) ? header : nullptr;
  if (hdr)
    memset(header, 0, sizeof(header));

  std::string path = fuzz_write_tmp(file, n, ".txt");
  vpArray2D<double> A;
  switch (mode / 2) {
  case 0: FUZZ_TRY(vpArray2D<double>::load(path, A, false, hdr)); break;
  case 1: FUZZ_TRY(vpArray2D<double>::load(path, A, true, hdr)); break;
  default: FUZZ_TRY(vpArray2D<double>::loadYAML(path, A, hdr)); break;
  }
  unlink(path.c_str());
  return 0;
}
