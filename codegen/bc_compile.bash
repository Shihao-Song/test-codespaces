BT=$1
TARGET=$(echo $BT | cut -d'.' -f 1)
LBT="${TARGET}.linked.bc"
LL="${TARGET}.ll"
O="${TARGET}.o"

llvm-dis $BT -o $LL
llvm-link $BT util/print.bc -o $LBT
llc -filetype=obj $LBT -o $O
clang $O -o $TARGET
