#ifdef NDEBUG
#error "MA2A test targets must keep assertions enabled even in Release builds"
#endif

#include <cassert>

int main() {
    bool evaluated = false;
    assert((evaluated = true));
    return evaluated ? 0 : 1;
}
