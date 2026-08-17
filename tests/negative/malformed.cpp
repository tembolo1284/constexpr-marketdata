#include "ctmd/snapshot.hpp"
#include "ctmd/schema.hpp"
static_assert(ctmd::SnapshotLike<decltype(ctmd::snapshot)>);
int main() { return 0; }
