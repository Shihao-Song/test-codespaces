BT=$1
TARGET=$(echo $BT | cut -d'.' -f 1)
LBT="${TARGET}.linked.bc"
LL="${TARGET}.ll"
O="${TARGET}.o"

rm -f $BT
rm -f $LBT
rm -f $O
rm -f $LL
rm -f $TARGET
