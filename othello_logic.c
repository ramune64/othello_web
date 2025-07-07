#include <stdio.h>
#include <stdint.h>
#include <math.h>
uint64_t white,black;
#define LEFT_MASK 0x7F7F7F7F7F7F7F7F
#define RIGHT_MASK 0xFEFEFEFEFEFEFEFE
#define safety_mask 0xFFFFFFFFFFFFFFFF

typedef struct {
    int direction;
    uint64_t mask;
} DirMask;

typedef struct {
    int row;
    int col;
} RawCol;


DirMask DIRECTIONS2[8] = {
    {-8,0xFFFFFFFFFFFFFFFF},  // 下
    {8,0xFFFFFFFFFFFFFFFF},   // 上
    {-1,LEFT_MASK},  // 右
    {1,RIGHT_MASK},   // 左
    {-9,LEFT_MASK},  // 右下
    {9,RIGHT_MASK},   // 左上
    {-7,RIGHT_MASK},  // 左下
    {7,LEFT_MASK}    // 右上
};



uint64_t shift_board(uint64_t board,int shift_value){
    """与えられた方向にボードをシフトする""";
    if (shift_value < 0){
        return board >> abs(shift_value) ; // 右シフト
    }
    else if (shift_value > 0){
        return board << abs(shift_value) ; // 左シフト
    }else{
        return board;
    }
}

int get_legal_square(const char* player_color,uint64_t current_board_w,uint64_t current_board_b){
    RawCol legal_list[64];
    if(player_color == "black"){
        current_board_w,current_board_b = current_board_b,current_board_w;
    }
    uint64_t empty;
    empty = ~(current_board_b | current_board_w)& 0xFFFFFFFFFFFFFFFF;
    for(int i;i<=8;i++){
        int direction = DIRECTIONS2[i].direction;
        uint64_t mask = DIRECTIONS2[i].mask;
        //printf("dirc=%llu\n",direction);
        //printf("mask=%llu\n",mask);
        printf("Direction: %d, Mask: 0x%016llX\n", direction, mask);
    }
    //legal_list = {};
    //printf("Hellow World");
    return 0;
}


int main(void){
    //printf("LEFT_MASK = %llu\n", LEFT_MASK&RIGHT_MASK);
    get_legal_square("white",0,0);
}