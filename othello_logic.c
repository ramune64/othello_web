#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>

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

int convert_l2n[26];  // 'a'～'z' に対応（全部で26文字）

void init_convert_l2n() {
    convert_l2n['a' - 'a'] = 1;
    convert_l2n['b' - 'a'] = 2;
    convert_l2n['c' - 'a'] = 3;
    convert_l2n['d' - 'a'] = 4;
    convert_l2n['e' - 'a'] = 5;
    convert_l2n['f' - 'a'] = 6;
    convert_l2n['g' - 'a'] = 7;
    convert_l2n['h' - 'a'] = 8;
}

void convert_act_str2bit(const char* act,int *bit_row,int *bit_col){
    init_convert_l2n();
    *bit_col = 7-(convert_l2n[act[0] - 'a'] - 1);
    *bit_row = 7-(act[1] - '1');
}

int bit_length(uint64_t x) {
    if (x == 0) return 0;  // 0は特別扱い（桁数0）
    
    int length = 0;
    while (x > 0) {
        x >>= 1;
        length++;
    }
    return length;
}

/* void convert_act_str2bit(act){
    put_pos_col = convert_l2n[act[0]]-1//文字を数字に変換してインデックスの表記に合わせる
    put_pos_row = int(act[1]) - 1//数字をインデックス表記に合わせる
    bit_row = 7-put_pos_row
    bit_col = 7-put_pos_col
} */

uint64_t shift_board(uint64_t board,int shift_value){
    if (shift_value < 0){
        return board >> abs(shift_value) ; // 右シフト
    }
    else if (shift_value > 0){
        return board << abs(shift_value) ; // 左シフト
    }else{
        return board;
    }
}

void get_legal_square(const char* player_color,uint64_t current_board_w,uint64_t current_board_b,RawCol *legal_list, int *legal_list_size){
    
    *legal_list_size = 0; 
    if(player_color == "black"){
        uint64_t temp = current_board_w;
        current_board_w = current_board_b;
        current_board_b = temp;
    }
    uint64_t empty;
    empty = ~(current_board_b | current_board_w)& 0xFFFFFFFFFFFFFFFF;
    uint64_t legal = 0,t,l_pos;
    int index,row,col;
    for(int i=0;i<8;i++){
        int direction = DIRECTIONS2[i].direction;
        uint64_t mask = DIRECTIONS2[i].mask;
        //printf("dirc=%llu\n",direction);
        //printf("mask=%llu\n",mask);
        //printf("Direction: %d, Mask: 0x%016llX\n", direction, mask);
        t = shift_board(current_board_w,direction) & mask & current_board_b & 0xFFFFFFFFFFFFFFFF;
        for(int k=0;k<5;k++){
            t |= shift_board(t,direction) & mask & current_board_b & 0xFFFFFFFFFFFFFFFF;
        }
        legal |= shift_board(t, direction) & mask & empty & 0xFFFFFFFFFFFFFFFF;
        
        while(legal>0){
            l_pos = legal & (uint64_t)(-((int64_t)legal));
            index = bit_length(l_pos) - 1;
            legal-=l_pos;
            //printf("%d",legal,"\n");
            row = index / 8;
            col = index % 8;
            if (*legal_list_size < 64) {
                legal_list[*legal_list_size] = (RawCol){row, col};
                (*legal_list_size)++;
            }
            //legal_list[*legal_list_size] = (RawCol){row,col};
            //(*legal_list_size) ++;
        }
    }
    
    //legal_list = {};
    //printf("Hellow World");
    //return legal_list;
}

void identify_flip_stone(const char* player_color,uint64_t current_board_w,uint64_t current_board_b,const char* action,int mode){
    if(player_color == "black"){
        uint64_t temp = current_board_w;
        current_board_w = current_board_b;
        current_board_b = temp;
    }
    RawCol legal_list[64];
    int legal_list_size;
    get_legal_square("white",current_board_w,current_board_b,legal_list,&legal_list_size);

    int index,bit_row=0,bit_col=0;
    uint64_t only_white;


    convert_act_str2bit(action,&bit_row,&bit_col);
    index = bit_row*8 + bit_col;
    //printf("%d,%d,%d\n",bit_row,bit_col,index);
}


int main(void){
    //printf("LEFT_MASK = %llu\n", LEFT_MASK&RIGHT_MASK);
    RawCol legal_list[64];
    int legal_list_size;
    //get_legal_square("black",68853694464,34628173824,legal_list,&legal_list_size);
    identify_flip_stone("black",68853694464,34628173824,"d3",1);
    //printf(legal_list);
    /* printf("\n");
    for (int i = 0; i < legal_list_size; i++) {
        printf("Legal Move: Row = %d, Col = %d\n", legal_list[i].row, legal_list[i].col);
    } */
    return 0;
}