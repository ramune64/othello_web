#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>
#include <omp.h> 

#define first_board_w 68853694464ULL
#define first_board_b 34628173824ULL

uint64_t white,black;
#define LEFT_MASK 0x7F7F7F7F7F7F7F7F// 左端を0にする
#define RIGHT_MASK 0xFEFEFEFEFEFEFEFE// 右端を0にする
#define safety_mask 0xFFFFFFFFFFFFFFFF

#define FALSE   0
#define TRUE    1

typedef struct {
    int direction;
    uint64_t mask;
} DirMask;

typedef struct {
    int row;
    int col;
} RowCol;

typedef struct {
    int row;
    int col;
    int color;
} RowColColor;

typedef struct{
    float score;
    RowCol move;
    uint64_t newBoardW;
    uint64_t newBoardB;
}MoveOrder;


int DIRECTIONS[8] = {
    -8,  // 下
    8,   // 上
    -1,  // 右
    1,   // 左
    -9,  // 右下
    9,   // 左上
    -7,  // 左下
    7    // 右上
};
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

RowCol cx_zone[12] = {
    {0,1},
    {1,1},
    {1,0},
    {0,6},
    {1,7},
    {1,6},
    {6,0},
    {6,1},
    {7,1},
    {6,7},
    {7,6},
    {6,6}};


int convert_l2n[26];  // 'a'～'z' に対応（全部で26文字）

int contains(int value,const int *list, int size) {
    for (int i = 0; i < size; i++) {
        if (list[i] == value) {
            return 1;
        }
    }
    return 0;
}

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

int bit_count(uint64_t x){
    if (x == 0) return 0;

    int count = 0;
    while(x>0){
        x &= (x - 1);
        count ++;
    }
    return count;
}

float nmax(float num1,float num2){
    if(num1>num2){
        return num1;
    }else{
        return num2;
    }
}
float nmin(float num1,float num2){
    if(num1<num2){
        return num1;
    }else{
        return num2;
    }
}

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

int is_blocked_direction(uint64_t white,uint64_t black,int direction,uint64_t now_bit,uint64_t con_white,uint64_t con_black){
    uint64_t all_stone = white|black;
    uint64_t all_con = con_white|con_black;
    uint64_t t = now_bit;

    while(TRUE){
        t = shift_board(t,direction);
        if (direction == -1 || direction == 7 || direction == -9){
            t &= LEFT_MASK;
        }else if(direction == 1 || direction == -7 || direction == 9){
            t &= RIGHT_MASK;
        }


        if (t==0){
            return TRUE;
        }

        if ((t & all_stone) == 0){
            return FALSE;
        }

        if (t & all_con){
            return TRUE;
        }
    }
}




void get_legal_square(const char* player_color,uint64_t current_board_w,uint64_t current_board_b,RowCol *legal_list, int *legal_list_size){
    //printf("\nget_legal");
    *legal_list_size = 0; 
    //printf("\nlegal1");
    if(strcmp(player_color, "black") == 0){
        uint64_t temp = current_board_w;
        current_board_w = current_board_b;
        current_board_b = temp;
        //printf("\nlegal1.5");
    }
    //printf("\nlegal2");
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
        
    }
    //printf("\nlegal3");
    while(legal>0){
        //printf("\nlegal4");
        l_pos = legal & (uint64_t)(-((int64_t)legal));
        if (l_pos == 0) break;
        index = bit_length(l_pos) - 1;
        legal-=l_pos;
        //printf("%d",legal,"\n");
        row = index / 8;
        col = index % 8;
        if (*legal_list_size < 64) {
            //printf("\nlegal5");
            legal_list[*legal_list_size] = (RowCol){row, col};
            (*legal_list_size)++;
        }
        //legal_list[*legal_list_size] = (RowCol){row,col};
        //(*legal_list_size) ++;
    }
    //printf("\nfinishやで");
    //printf("\nlegal_list_size: %d\n", *legal_list_size);
    return;
    //legal_list = {};
    //printf("Hellow World");
    //return legal_list;
}


void identify_flip_stone(const char* player_color,uint64_t *board_w,uint64_t *board_b,RowCol action,int mode,RowCol *flip_list,int *flip_list_size){
    uint64_t current_board_w=*board_w,current_board_b=*board_b;
    *flip_list_size = 0;
    if(strcmp(player_color, "black") == 0){
        
        uint64_t temp = current_board_w;
        current_board_w = current_board_b;
        current_board_b = temp;
    }
    /* RowCol legal_list[64];
    int legal_list_size;
    get_legal_square("white",current_board_w,current_board_b,legal_list,&legal_list_size); */

    int index,bit_row=0,bit_col=0,row,col;
    uint64_t only_white,flips=0,flip_candidate,t,next_shifted,f_pos;


    //convert_act_str2bit(action,&bit_row,&bit_col);
    bit_row = action.row;
    bit_col = action.col;
    index = bit_row*8 + bit_col;
    only_white = ((uint64_t)1)  << index;
    //printf("\n%d,%d,%d\n",bit_row,bit_col,index);
    for(int i=0;i<8;i++){
        int direction = DIRECTIONS2[i].direction;
        uint64_t mask = DIRECTIONS2[i].mask;
        flip_candidate = 0;
        t = shift_board(only_white,direction) & mask & current_board_b & 0xFFFFFFFFFFFFFFFF;
        flip_candidate |= t;
        while(t>0){
            next_shifted = shift_board(t, direction) & mask & 0xFFFFFFFFFFFFFFFF;
            if((next_shifted & current_board_w) > 0){
                flips |= flip_candidate;
                //printf("%llu\n",flips);
                break;
            }
            t = next_shifted & current_board_b;
            flip_candidate |= t;
        }
    }
    
    //current_board_b = current_board_b ^ flips;
    current_board_b ^= flips;
    current_board_w |= flips;
    current_board_w |= only_white;
    if(strcmp(player_color, "black") == 0){
        uint64_t temp = current_board_w;
        current_board_w = current_board_b;
        current_board_b = temp;
    }
    *board_w = current_board_w;
    *board_b = current_board_b;
    if(mode!=0){
        while(flips>0){
            f_pos = flips & (uint64_t)(-((int64_t)flips));
            index = bit_length(f_pos) - 1;
            flips-=f_pos;
            //printf("%d",legal,"\n");
            row = index / 8;
            col = index % 8;
            if (*flip_list_size < 64) {
                flip_list[*flip_list_size] = (RowCol){row, col};
                (*flip_list_size)++;
            }
            //legal_list[*legal_list_size] = (RowCol){row,col};
            //(*legal_list_size) ++;
        }
    }
}

RowCol corners[4] = {
    {0,0},{0,7},{7,0},{7,7}
};
uint64_t m1_e = 0x7E;
uint64_t m1_c = 0x81;
uint64_t m3_e = 0x7E00000000000000;
uint64_t m3_c = 0x8100000000000000;
uint64_t m4_e = 0x0001010101010100;
uint64_t m4_c = 0x0100000000000001;
uint64_t m2_e = 0x0080808080808000;
uint64_t m2_c = 0x8000000000000080;


void get_confirmed_stones_lite(uint64_t board_w,uint64_t board_b,uint64_t *con_white,uint64_t *con_black){
    uint64_t m_e_list[4] = {m1_e,m2_e,m3_e,m4_e};
    uint64_t m_c_list[4] = {m1_c,m2_c,m3_c,m4_c};
    uint64_t black_confirmed=0,white_confirmed=0,confirmed_all,corner_bit,now_bit,shifted,adjust_board_w,adjust_board_b;
    confirmed_all = white_confirmed | black_confirmed;
    RowColColor queue[4];
    int queue_length = 0,index;

    for(int i=0;i<4;i++){
        int c_row = corners[i].row;
        int c_col = corners[i].col;
        index = c_row*8+c_col;
        corner_bit = ((uint64_t)1) << index;
        if(corner_bit&board_w){
            confirmed_all |= corner_bit;
            white_confirmed |= corner_bit;
            queue[queue_length] = (RowColColor){c_row,c_col,1};
            queue_length++;
        }else if(corner_bit&board_b){
            confirmed_all |= corner_bit;
            black_confirmed |= corner_bit;
            queue[queue_length] = (RowColColor){c_row,c_col,-1};
            queue_length++;
        }
    }

    for(int i=0;i<queue_length;i++){
        int row = queue[i].row,col = queue[i].col,origin_color = queue[i].color;
        for(int k=0;k<4;k++){
            int direction = DIRECTIONS[k];
            int last_color = origin_color;
            index = row*8 + col;
            now_bit = ((uint64_t)1) << index;

            while(TRUE){
                if (((now_bit&~LEFT_MASK&0xFFFFFFFFFFFFFFFF)>0 && (direction == 1 || direction == -7 || direction == 9))||((now_bit&~RIGHT_MASK&0xFFFFFFFFFFFFFFFF)>0 && (direction == -1 || direction == 7 || direction == -9))){
                    break;
                }
                shifted = shift_board(now_bit,direction);
                adjust_board_w = shifted & board_w;
                adjust_board_b = shifted & board_b;
                now_bit = shifted;
                if(adjust_board_w>0 && last_color == 1){
                    confirmed_all |= shifted;
                    white_confirmed |= shifted;
                    last_color = 1;
                }else if(adjust_board_b && last_color == -1){
                    confirmed_all |= shifted;
                    black_confirmed |= shifted;
                    last_color = -1;
                }else{
                    break;
                }
            }
        }
    }
    for(int i=0;i<4;i++){
        uint64_t white_c = board_w & m_c_list[i];
        uint64_t black_c = board_b & m_c_list[i];
        uint64_t black_e = board_b & m_e_list[i];
        uint64_t white_e = board_w & m_e_list[i];
        if (bit_count(white_c|black_c|black_e|white_e) == 8){
            black_e = board_b & m_e_list[i];
            white_e = board_w & m_e_list[i];
            black_confirmed |= black_e;
            confirmed_all |= black_e;
            white_confirmed |= white_e;
            confirmed_all |= white_e;
        }
    }
    *con_white = white_confirmed;
    *con_black = black_confirmed;
}

void get_confirmed_stones(uint64_t board_w,uint64_t board_b,uint64_t *con_white,uint64_t *con_black){
    uint64_t m_e_list[4] = {m1_e,m2_e,m3_e,m4_e};
    uint64_t m_c_list[4] = {m1_c,m2_c,m3_c,m4_c};
    uint64_t black_confirmed=0,white_confirmed=0,confirmed_all,corner_bit,now_bit,shifted,adjust_board_w,adjust_board_b;
    confirmed_all = white_confirmed | black_confirmed;
    RowColColor queue[4];
    int queue_length = 0,index;

    for(int i=0;i<4;i++){
        int c_row = corners[i].row;
        int c_col = corners[i].col;
        index = c_row*8+c_col;
        corner_bit = ((uint64_t)1) << index;
        if(corner_bit&board_w){
            confirmed_all |= corner_bit;
            white_confirmed |= corner_bit;
            queue[queue_length] = (RowColColor){c_row,c_col,1};
            queue_length++;
        }else if(corner_bit&board_b){
            confirmed_all |= corner_bit;
            black_confirmed |= corner_bit;
            queue[queue_length] = (RowColColor){c_row,c_col,-1};
            queue_length++;
        }
    }

    for(int i=0;i<queue_length;i++){
        int row = queue[i].row,col = queue[i].col,origin_color = queue[i].color;
        for(int k=0;k<4;k++){
            int direction = DIRECTIONS[k];
            int last_color = origin_color;
            index = row*8 + col;
            now_bit = ((uint64_t)1) << index;

            while(TRUE){
                if (((now_bit&~LEFT_MASK&0xFFFFFFFFFFFFFFFF)>0 && (direction == 1 || direction == -7 || direction == 9))||((now_bit&~RIGHT_MASK&0xFFFFFFFFFFFFFFFF)>0 && (direction == -1 || direction == 7 || direction == -9))){
                    break;
                }
                shifted = shift_board(now_bit,direction);
                adjust_board_w = shifted & board_w;
                adjust_board_b = shifted & board_b;
                now_bit = shifted;
                if(adjust_board_w>0 && last_color == 1){
                    confirmed_all |= shifted;
                    white_confirmed |= shifted;
                    last_color = 1;
                }else if(adjust_board_b && last_color == -1){
                    confirmed_all |= shifted;
                    black_confirmed |= shifted;
                    last_color = -1;
                }else{
                    break;
                }
            }
        }
    }
    for(int i=0;i<4;i++){
        uint64_t white_c = board_w & m_c_list[i];
        uint64_t black_c = board_b & m_c_list[i];
        uint64_t black_e = board_b & m_e_list[i];
        uint64_t white_e = board_w & m_e_list[i];
        if (bit_count(white_c|black_c|black_e|white_e) == 8){
            black_e = board_b & m_e_list[i];
            white_e = board_w & m_e_list[i];
            black_confirmed |= black_e;
            confirmed_all |= black_e;
            white_confirmed |= white_e;
            confirmed_all |= white_e;
        }
    }
    int check_list[4];
    int check_list_length = 0; 
    uint64_t now_bit2,last_color2,shifted2,adjust_board_w2,adjust_board_b2;
    for(int i=0;i<queue_length;i++){
        int row = queue[i].row,col = queue[i].col,origin_color = queue[i].color;
        for(int k=0;k<4;k++){
            int direction = DIRECTIONS[k+4];
            //printf("%d\n",direction);
            int last_color = origin_color;
            index = row*8 + col;
            now_bit = ((uint64_t)1) << index;
            
            while(TRUE){
                
                if (((now_bit&~LEFT_MASK&0xFFFFFFFFFFFFFFFF)>0 && (direction == 1 || direction == -7 || direction == 9))||((now_bit&~RIGHT_MASK&0xFFFFFFFFFFFFFFFF)>0 && (direction == -1 || direction == 7 || direction == -9))){
                    break;
                }
                shifted = shift_board(now_bit,direction);
                adjust_board_w = shifted & board_w;
                adjust_board_b = shifted & board_b;
                now_bit = shifted;
                
                if (adjust_board_w>0 || adjust_board_b>0){
                    if (adjust_board_w>0){
                        if ((white_confirmed & adjust_board_w)>0){
                            break;
                        }
                        last_color = 1;
                    }
                    if (adjust_board_b>0){
                        if ((white_confirmed & adjust_board_b)>0){
                            break;
                        }
                        last_color = -1;
                    }
                    for(int l=0;l<4;l++){
                        check_list[l] = -1;
                    }
                    check_list_length = 0; 
                    for(int j=0;j<8;j++){
                        int direction2 = DIRECTIONS[j];
                        
                        now_bit2 = now_bit;
                        last_color2 = last_color;
                        
                        while (TRUE){
                            shifted2 = shift_board(now_bit2,direction2);
                            adjust_board_w2 = shifted2 & board_w;
                            adjust_board_b2 = shifted2 & board_b;
                            now_bit2 = shifted2;
                            if (adjust_board_w2 || adjust_board_b2){
                                if (last_color2 == 1){
                                    if ((white_confirmed&adjust_board_w2)>0){
                                    if (abs(direction2) == 8){//上下方向の移動なら
                                        check_list[check_list_length]=2;
                                    }else if( abs(direction2) == 1){
                                        check_list[check_list_length]=1;
                                    }else if (abs(direction2) == 7){//右上左下方向の移動なら
                                        check_list[check_list_length]=3;
                                    }else if (abs(direction2) == 9){//右下左上方向の移動なら
                                        check_list[check_list_length]=4;
                                    }
                                    check_list_length++;
                                    last_color2 = 1;
                                    break;
                                    }else{
                                        last_color2 = last_color;
                                        break;
                                    }
                                }else{
                                    if( (black_confirmed&adjust_board_b2)>0){
                                        if (abs(direction2) == 8){//上下方向の移動なら
                                        check_list[check_list_length]=2;
                                        }else if( abs(direction2) == 1){
                                            check_list[check_list_length]=1;
                                        }else if (abs(direction2) == 7){//右上左下方向の移動なら
                                            check_list[check_list_length]=3;
                                        }else if (abs(direction2) == 9){//右下左上方向の移動なら
                                            check_list[check_list_length]=4;
                                        }
                                        check_list_length++;
                                        last_color2 = -1;
                                        break;
                                    }else{
                                            last_color2 = last_color;
                                            break;
                                    }
                                    
                                }
                            }else if((shifted2==0) || bit_length(shifted2) > 64 || ((direction == -1 || direction == 7 || direction == -9) && (shifted2&~LEFT_MASK&0xFFFFFFFFFFFFFFFF)>0) || ((direction == 1 || direction == -7 || direction == 9) && (shifted2&~RIGHT_MASK&0xFFFFFFFFFFFFFFFF)>0)){
                                if (abs(direction2) == 8){//上下方向の移動なら
                                check_list[check_list_length]=2;
                                }else if( abs(direction2) == 1){
                                    check_list[check_list_length]=1;
                                }else if (abs(direction2) == 7){//右上左下方向の移動なら
                                    check_list[check_list_length]=3;
                                }else if (abs(direction2) == 9){//右下左上方向の移動なら
                                    check_list[check_list_length]=4;
                                }
                                check_list_length++;
                                break;
                            }else{
                                break;
                            }
                        }
                    }
                    /* printf("%d",check_list[0]);
                    printf("%d",check_list[1]);
                    printf("%d",check_list[2]);
                    printf("%d\n",check_list[3]); */
                    
                    if ((contains(1,check_list,check_list_length)) && (contains(2,check_list,check_list_length)) && (contains(3,check_list,check_list_length)) && (contains(4,check_list,check_list_length))){
                        if (adjust_board_w>0){
                            confirmed_all |= adjust_board_w;
                            white_confirmed |= adjust_board_w;
                            last_color = 1;
                        }else{
                            confirmed_all |= adjust_board_b;
                            black_confirmed |= adjust_board_b;
                            last_color = -1;
                        }
                    }
                }else{
                    break;
                }
            }
            
        }
    }
    uint64_t all_white[64],all_black[64],get_all_white,get_all_black,position,only_white,only_black,shifted_masked;
    int all_white_length = 0,all_black_length = 0,direction;
    get_all_white = board_w;
    get_all_black = board_b;
    for(int i=0;i<4;i++){//辺以外の確定石
        //printf("\n--------------------------------------\n");
        all_white_length = 0;
        all_black_length = 0;
        get_all_white = board_w;
        get_all_black = board_b;
        
        while (get_all_white>0){
            position = get_all_white & -get_all_white;
            get_all_white-=position;
            index = bit_length(position) - 1;
            //printf("\n%d\n",index);
            only_white = ((uint64_t)1) << index;
            if ((only_white&white_confirmed)==0){
                
                all_white[all_white_length] = only_white;
                all_white_length++;
            }
        }
        //printf("\n%d個\n",all_white_length);
        while (get_all_black>0){
            position = get_all_black & -get_all_black;
            
            get_all_black-=position;
            index = bit_length(position) - 1;
            only_black = ((uint64_t)1) << index;
            
            if ((only_black&black_confirmed)==0){
                /* printf("\n%llu",position);
                printf("\n%d\n",index);
                printf("\n%d\n",bit_length(only_black));
                printf("\n%llu",only_black); */
                all_black[all_black_length] = only_black;
                all_black_length++;
            }
        }
        
        for(int k=0;k<all_white_length;k++){
            int check_list[16];
            int check_list_length = 0;
            if((i&1)==1){
                only_white = all_white[all_white_length-k-1];
            }else{
                only_white = all_white[k];
            }
            
            now_bit = only_white;
            for(int j=0;j<8;j++){
                direction = DIRECTIONS[j];
                now_bit = only_white;
                shifted = shift_board(now_bit,direction);
                if (direction == -1 || direction == 7 || direction == -9){
                    shifted_masked = shifted & LEFT_MASK;
                }else if(direction == 1 || direction == -7 || direction == 9){
                    shifted_masked = shifted & RIGHT_MASK;
                }else{
                    shifted_masked = shifted;
                }
                adjust_board_w = shifted_masked & board_w;
                adjust_board_b = shifted_masked & board_b;

                //printf("%d",is_blocked_direction(board_w,board_b,direction,only_white,white_confirmed,black_confirmed));
                
                if (((black_confirmed & adjust_board_b)>0) || (is_blocked_direction(board_w,board_b,direction,only_white,white_confirmed,black_confirmed))){
                    if (abs(direction) == 8){//上下方向の移動なら
                        check_list[check_list_length]=6;
                    }else if( abs(direction) == 1){
                        check_list[check_list_length]=5;
                    }else if (abs(direction) == 7){//右上左下方向の移動なら
                        check_list[check_list_length]=7;
                    }else if (abs(direction) == 9){//右下左上方向の移動なら
                        check_list[check_list_length]=8;
                    }
                    check_list_length++;
                }
                if ((shifted_masked == 0) || (bit_length(shifted_masked) > 64) || (white_confirmed & adjust_board_w) != 0){
                    if (abs(direction) == 8){//上下方向の移動なら
                        check_list[check_list_length]=2;
                    }else if( abs(direction) == 1){
                        check_list[check_list_length]=1;
                    }else if (abs(direction) == 7){//右上左下方向の移動なら
                        check_list[check_list_length]=3;
                    }else if (abs(direction) == 9){//右下左上方向の移動なら
                        check_list[check_list_length]=4;
                    }
                    check_list_length++;
                }
                /* if(is_blocked_direction(board_w,board_b,direction,only_white,white_confirmed,black_confirmed) && bit_length(only_white)==52){
                    printf("\n%d,%d",bit_length(only_white),direction);
                    for(int idx=0;idx<check_list_length;idx++){
                        printf("\n%d,",check_list[idx]);
                    }
                } */
            }
            for(int n=5;n<9;n++){
                int target_num_count = 0;
                int target_idx_num1=-1,target_idx_num2=-1;
                for(int idx=0;idx<check_list_length;idx++){
                    if(check_list[idx] == n){
                        target_idx_num1 = target_idx_num2;
                        target_idx_num2 = idx;
                        target_num_count ++;
                    }
                }
                if(target_num_count == 2){
                    check_list[target_idx_num1] = n-4;
                    check_list[target_idx_num2] = n-4;
                }
            }
            //printf("\n%d,%d\n",check_list_length,bit_length(only_white)-1);
            if ((contains(1,check_list,check_list_length)) && (contains(2,check_list,check_list_length)) && (contains(3,check_list,check_list_length)) && (contains(4,check_list,check_list_length))){
                //printf("%d,%d\n",check_list_length,bit_length(only_white)-1);    
                confirmed_all |= only_white;
                white_confirmed |= only_white;
            }
        }


        
        for(int k=0;k<all_black_length;k++){
            int check_list[8];
            int check_list_length = 0;
            if((i&1)==1){
                only_black = all_black[all_black_length-k-1];
            }else{
                only_black = all_black[k];
            }
            
            now_bit = only_black;
            
            for(int j=0;j<8;j++){
                direction = DIRECTIONS[j];
                now_bit = only_black;
                shifted = shift_board(now_bit,direction);
                if (direction == -1 || direction == 7 || direction == -9){
                    shifted_masked = shifted & LEFT_MASK;
                }else if(direction == 1 || direction == -7 || direction == 9){
                    shifted_masked = shifted & RIGHT_MASK;
                }else{
                    shifted_masked = shifted;
                }
                adjust_board_w = shifted_masked & board_w;
                adjust_board_b = shifted_masked & board_b;

                if ((black_confirmed & adjust_board_w)>0 || is_blocked_direction(board_w,board_b,direction,only_black,white_confirmed,black_confirmed)){
                    if (abs(direction) == 8){//上下方向の移動なら
                        check_list[check_list_length]=6;
                    }else if( abs(direction) == 1){
                        check_list[check_list_length]=5;
                    }else if (abs(direction) == 7){//右上左下方向の移動なら
                        check_list[check_list_length]=7;
                    }else if (abs(direction) == 9){//右下左上方向の移動なら
                        check_list[check_list_length]=8;
                    }
                    check_list_length++;
                }
                if ((shifted_masked == 0) || (bit_length(shifted_masked) > 64) || (black_confirmed & adjust_board_b) != 0){
                    if (abs(direction) == 8){//上下方向の移動なら
                        check_list[check_list_length]=2;
                    }else if( abs(direction) == 1){
                        check_list[check_list_length]=1;
                    }else if (abs(direction) == 7){//右上左下方向の移動なら
                        check_list[check_list_length]=3;
                    }else if (abs(direction) == 9){//右下左上方向の移動なら
                        check_list[check_list_length]=4;
                    }
                    check_list_length++;
                }
            }
            for(int n=5;n<9;n++){
                int target_num_count = 0;
                int target_idx_num1=-1,target_idx_num2=-1;
                for(int idx=0;idx<check_list_length;idx++){
                    if(check_list[idx] == n){
                        target_idx_num1 = target_idx_num2;
                        target_idx_num2 = idx;
                        target_num_count ++;
                    }
                }
                if(target_num_count == 2){
                    check_list[target_idx_num1] = n-4;
                    check_list[target_idx_num2] = n-4;
                }
            }
            //printf("\n%d\n%d\n",check_list_length,bit_length(only_black));
            if ((contains(1,check_list,check_list_length)) && (contains(2,check_list,check_list_length)) && (contains(3,check_list,check_list_length)) && (contains(4,check_list,check_list_length))){
                    confirmed_all |= only_black;
                    black_confirmed |= only_black;
            }
        }
    }
    *con_white = white_confirmed;
    *con_black = black_confirmed;
    //return white_confirmed,black_confirmed;
}


int scores[64] = {
     30, -12,   0,  -1,  -1,   0, -12, 30,
    -12, -20,  -3,  -3,  -3,  -3, -20,-12,
      0,  -3,   0,  -1,  -1,   0,  -3,  0,
     -1,  -3,  -1,  -1,  -1,  -1,  -3, -1,
     -1,  -3,  -1,  -1,  -1,  -1,  -3, -1,
      0,  -3,   0,  -1,  -1,   0,  -3,  0,
    -12, -20,  -3,  -3,  -3,  -3, -20,-12,
     30, -12,   0,  -1,  -1,   0, -12, 30
};




void eval_bitboard_score(uint64_t board_w,uint64_t board_b,int *white_score,int *black_score){
    *black_score = 0;
    *white_score = 0;
    for(int i=0;i<64;i++){
        uint64_t mask = ((uint64_t)1)<<i;
        if((board_b & mask)>0){
            *black_score += scores[i];
        }else if((board_w & mask)>0){
            *white_score += scores[i];
        }
    }
}

float calc_spread_penalty(uint64_t board) {
    int count = 0;
    float sum_x = 0, sum_y = 0;
    for (int i = 0; i < 64; i++) {
        if ((board >> i) & 1) {
            int x = i % 8, y = i / 8;
            sum_x += x;
            sum_y += y;
            count++;
        }
    }
    if (count == 0) return 0;
    float cx = sum_x / count, cy = sum_y / count;

    float var = 0;
    for (int i = 0; i < 64; i++) {
        if ((board >> i) & 1) {
            int x = i % 8, y = i / 8;
            float dx = x - cx, dy = y - cy;
            var += dx * dx + dy * dy;
        }
    }
    return var/count;
}

void find_connected_open_stone(uint64_t board, uint64_t empty, float *count, float *count_open) {
    *count = 0;
    *count_open = 0;
    for (int i = 0; i < 64; i++) {
        if ((board >> i) & 1) {
            uint64_t pos = ((uint64_t)1) << i;

            uint64_t neighbor_mask =
                ((pos << 1) & RIGHT_MASK) |  // 左
                ((pos >> 1) & LEFT_MASK)  |  // 右
                (pos << 8)                |  // 上
                (pos >> 8)                |  // 下
                ((pos << 7) & RIGHT_MASK) |  // 左上
                ((pos << 9) & LEFT_MASK)  |  // 右上
                ((pos >> 7) & LEFT_MASK)  |  // 右下
                ((pos >> 9) & RIGHT_MASK);   // 左下

            uint64_t connected_pairs = neighbor_mask & board;
            uint64_t open_stones = neighbor_mask & empty;

            *count += bit_count(connected_pairs);
            *count_open += bit_count(open_stones);
        }
    }
}
void find_connected_open_stone2(uint64_t board_w,uint64_t board_b, uint64_t empty, float *count, float *count_open, float *touch_other) {
    *count = 0;
    *count_open = 0;
    *touch_other = 0;
    for (int i = 0; i < 64; i++) {
        if ((board_w >> i) & 1) {
            uint64_t pos = ((uint64_t)1) << i;

            uint64_t neighbor_mask =
                ((pos << 1) & RIGHT_MASK) |  // 左
                ((pos >> 1) & LEFT_MASK)  |  // 右
                (pos << 8)                |  // 上
                (pos >> 8)                |  // 下
                ((pos << 7) & RIGHT_MASK) |  // 左上
                ((pos << 9) & LEFT_MASK)  |  // 右上
                ((pos >> 7) & LEFT_MASK)  |  // 右下
                ((pos >> 9) & RIGHT_MASK);   // 左下

            uint64_t touch_other_pairs = neighbor_mask & board_b;
            uint64_t connected_pairs = neighbor_mask & board_w;
            uint64_t open_stones = neighbor_mask & empty;

            *count += (float)bit_count(connected_pairs);
            *count_open += (float)bit_count(open_stones);
            *touch_other += (float)bit_count(touch_other_pairs);
        }
    }
}
uint64_t masks[56] = {
    1161999622361579520ULL,
    2151686160ULL,
    16909320ULL,
    577588855528488960ULL,
    580999813328273408ULL,
    550831656968ULL,
    4328785936ULL,
    1155177711073755136ULL,
    290499906672525312ULL,
    141012904183812ULL,
    1108169199648ULL,
    2310355422147575808ULL,
    145249953336295424ULL,
    36099303471055874ULL,
    283691315109952ULL,
    4620710844295151872ULL,
    72624976668147840ULL,
    9241421688590303745ULL,
    72624976668147840ULL,
    9241421688590303745ULL,
    71776119061217280ULL,
    4629771061636907072ULL,
    65280ULL,
    144680345676153346ULL,
    280375465082880ULL,
    2314885530818453536ULL,
    16711680ULL,
    289360691352306692ULL,
    1095216660480ULL,
    1157442765409226768ULL,
    4278190080ULL,
    578721382704613384ULL,
    18393263828134526976ULL,
    9277556521783312512ULL,
    17151ULL,
    72903122791498497ULL,
    17940089115630370816ULL,
    827867578560ULL,
    7967ULL,
    217020518514032640ULL,
    16204197749883666432ULL,
    14737632ULL,
    460551ULL,
    506381179683864576ULL,
    17924467806326226944ULL,
    551911735544ULL,
    4311810847ULL,
    2234630943929270272ULL,
    4340344140878315520ULL ,
    211934100062208ULL ,
    15420ULL ,
    3311470313472ULL ,
    17357084619874238464ULL ,
    2160124144ULL ,
    16975631ULL ,
    1082837538235744256ULL
};
float counts[56] = {
        4 ,4 ,4 ,4 ,
        5 ,5 ,5 ,5 ,
        6 ,6 ,6 ,6 ,
        7 ,7 ,7 ,7 ,
        8 ,8 ,8 ,8 ,
        8 ,8 ,8 ,8 ,
        8 ,8 ,8 ,8 ,
        8 ,8 ,8 ,8 ,
        10 ,10 ,10 ,10 ,
        10 ,10 ,10 ,10 ,
        9 ,9 ,9 ,9 ,
        10 ,10 ,10 ,10 ,
        8 ,8 ,8 ,8 ,
        10 ,10 ,10 ,10
};

uint64_t edge_pattern[8] = {
    9223513326254850048ULL,
    190ULL,
    282578800148481ULL,
    9007199254740992000ULL,
    36170086419005568ULL,
    125ULL,
    72058697861366016ULL,
    13690942867206307840ULL
};

float position_point(uint64_t board){
    float score = 0;
    for(int i=0;i<48;i++){
        if((board&masks[i])==masks[i]){
            score += counts[i];
        }
    }
    return score;
}
float evaluate_board5(uint64_t board_w,uint64_t board_b,float true_pass,float false_pass){
    
    uint64_t empty =  ~(board_w | board_b)& 0xFFFFFFFFFFFFFFFF;
    int turn = 60 - bit_count(empty);
    float alpha = nmin(nmax(0,(turn -40) / 10),1);
    float beta = nmin(nmax(0,(turn -25) / 15),1);
    int con_weight=100;
    
    float spread_score = 0,open_stones_score = 0,connected_pairs_score = 0;
    float b_snum = bit_count(board_b);
    float w_snum = bit_count(board_w);
    float connect_w_num,connect_b_num,open_w_num,open_b_num,touch_w2b,touch_b2w,touch_other;
    find_connected_open_stone2(board_w,board_b,empty,&connect_w_num,&open_w_num,&touch_w2b);
    find_connected_open_stone2(board_b,board_w,empty,&connect_b_num,&open_b_num,&touch_b2w);
    float spread_w = calc_spread_penalty(board_w);
    float spread_b = calc_spread_penalty(board_b);
    float open_stones_w_num = open_w_num/(w_snum+0.1);
    float open_stones_b_num = open_b_num/(b_snum+0.1);
    if(beta!=1){
        

        
        

        

        

        

        

        
        open_stones_score = -open_stones_w_num + open_stones_b_num;
        spread_score = -spread_w + spread_b;
    }
    touch_other = touch_w2b/(w_snum+0.1) - touch_b2w/(b_snum+0.1);
    float connected_pairs_w_num = connect_w_num/(w_snum+0.1);
    float connected_pairs_b_num = connect_b_num/(b_snum+0.1);
    printf("\nopen_w:%f\nspread_w:%f\ntouch_w2b:%f\nconnect_w:%f",open_stones_w_num,spread_w,touch_w2b/(w_snum+0.1),connected_pairs_w_num);
    connected_pairs_score = connected_pairs_w_num - connected_pairs_b_num;
    RowCol legal_list_w[64];
    //RowCol *legal_list_w = malloc(sizeof(RowCol) * 64);
    //RowCol *legal_list_b = malloc(sizeof(RowCol) * 64);
    int legal_list_size_w=0;
    get_legal_square("white",board_w,board_b,legal_list_w,&legal_list_size_w);
    //printf("\nwhite_legal_finish");
    RowCol legal_list_b[64];
    int legal_list_size_b=0;
    //printf("\nblack_legal_start");
    get_legal_square("black",board_w,board_b,legal_list_b,&legal_list_size_b);
    //printf("\nlegal_list_size_b: %d\n", legal_list_size_b);
    //printf("\nblack_finish");
    //
    //printf("\ndef_cx");
    //
    int w_cx = 0,b_cx = 0,index=0;
    uint64_t w_legal=0,b_legal=0,legal_bit;
    //printf("\ndef_cx2");
    //
    
    for(int idx=0;idx<legal_list_size_w;idx++){
        RowCol legal = legal_list_w[idx];
        int row=legal.row,col=legal.col; 
        index = row*8 + col;
        legal_bit = ((uint64_t)1)  << index;
        w_legal |= legal_bit;
        for(int cx_idx=0;cx_idx<12;cx_idx++){
            RowCol cx = cx_zone[cx_idx];
            int cx_row = cx.row,cx_col = cx.col;
            if(cx_row == row && cx_col == col){
                w_cx++;
            }
        }
    }
    //
    for(int idx=0;idx<legal_list_size_b;idx++){
        RowCol legal = legal_list_b[idx];
        int row=legal.row,col=legal.col;
        index = row*8 + col;
        legal_bit = ((uint64_t)1)  << index;
        b_legal |= legal_bit;
        for(int cx_idx=0;cx_idx<12;cx_idx++){
            RowCol cx = cx_zone[cx_idx];
            int cx_row = cx.row,cx_col = cx.col;
            if(cx_row == row && cx_col == col){
                b_cx++;
            }
        }
    }
    //
    float zennmetu_keikoku = 0;
    if(bit_count(board_w) <= 2&&turn>=8){
        zennmetu_keikoku = -6000;
    }
    //
    float edge_point_w = 0;
    float edge_point_b = 0;
    float edge_point,dis_num=0;
    uint64_t m_e_list[4] = {m1_e,m2_e,m3_e,m4_e};
    uint64_t m_c_list[4] = {m1_c,m2_c,m3_c,m4_c};
    uint64_t c_w=0,c_b=0;
    if(turn>=40){
        get_confirmed_stones(board_w,board_b,&c_w,&c_b);
    }else{
        get_confirmed_stones_lite(board_w,board_b,&c_w,&c_b);
    }

    int c_w_num=bit_count(c_w);
    int c_b_num=bit_count(c_b);
    float con_score = (c_w_num - c_b_num);
    //
    for(int i=0;i<4;i++){
        int dec_point=0,add_point=0;

        int w_num=0,b_num=0;

        w_num = bit_count(board_w&m_e_list[i]);
        b_num = bit_count(board_b&m_e_list[i]);
        uint64_t w_corner = board_w&m_c_list[i];
        uint64_t b_corner = board_b&m_c_list[i];
        uint64_t w_corner_legal = w_legal&m_c_list[i];
        uint64_t b_corner_legal = b_legal&m_c_list[i];
        int b_c_num = bit_count(b_corner);
        int w_c_num = bit_count(w_corner);
        int w_c_legal_num = bit_count(w_corner_legal);
        int b_c_legal_num = bit_count(b_corner_legal);
        int edge_con_num_w = bit_count(m_e_list[i]&c_w);
        int edge_con_num_b = bit_count(m_e_list[i]&c_b);
        //printf("\nw:%d\nb:%d",w_num,b_num);
        if(w_num==6){
            int dec = 0;
            if(b_c_num==1&&w_c_num==0){
                dec_point += 6*con_weight*0.9;
                dec = 1;
            }
            if(b_c_legal_num>0){
                dec_point += 6*con_weight* 0.8;
                dec = 1;
            }
            if(b_c_num==0){
                add_point += 6*con_weight* 0.3;
            }
        }
        
        /* if((w_num+b_num)==6&&w_c_num<=1&&b_num!=0){
            dec_point += (6-edge_con_num_w)*con_weight*0.9;
        } */
        if((w_num+b_num)==6&&b_num!=0){
            if(w_c_num<=1){
                dec_point += (6-edge_con_num_w)*con_weight*0.9;
            }
            if((edge_con_num_w+edge_con_num_b)<=5&&w_c_num==1&&w_c_legal_num==1){
                dec_point += (b_num-edge_con_num_b)*con_weight*0.9;
            }
        }
        //printf("\nadd:%d\ndec:%d",add_point,dec_point);
        edge_point_w += add_point;// + w_c_legal_num*6*0.4;
        edge_point_w -= dec_point;
        dec_point = 0;
        add_point = 0;
        /* if(b_num<=3){
            if(b_c_legal_num>=1&&(b_num+w_num)>=5){
                edge_point_w -= w_num*con_weight/10;
            }else{
                if(b_num<=0){
                    edge_point_w += w_num*con_weight/20;
                }
            }
        } */
        /* if(b_num<=0){
            edge_point_w += w_num*con_weight/10;
        }else{
            edge_point_w -= w_num*con_weight/10;
        } */
        if(b_num==6){
            int dec = 0;
            if(w_c_num==1&&b_c_num==0){
                dec_point += 6*con_weight*0.9;
                dec = 1;
                //printf("\n1:%f",10*6*con_weight* (4.0/5)/10);
            }
            if(w_c_legal_num>0){
                dec_point += 6*con_weight* 0.8;
                dec = 1;
                //printf("\n2:%f",10*6*con_weight* (3.0/5)/10);
            }
            if(dec==0){
                add_point += 6*con_weight* 0.3;
                //printf("\n3:%f",10*6*con_weight* (4.0/5)/10);
            }
        }
        /* if((w_num+b_num)==6&&b_c_num<=1&&w_num!=0){
            dec_point += (6-edge_con_num_b)*con_weight*0.9;
        } */
        if((w_num+b_num)==6&&w_num!=0){
            if(b_c_num<=1){
                dec_point += (6-edge_con_num_b)*con_weight*0.9;
            }
            if((edge_con_num_b+edge_con_num_w)<=5&&b_c_num==1&&b_c_legal_num==1){
                dec_point += (w_num-edge_con_num_w)*con_weight*0.9;
            }
        }
        edge_point_b += add_point;// + b_c_legal_num*6*0.4;
        edge_point_b -= dec_point;
        /* if(w_num<=3){
            if((w_c_legal_num>=1&&(b_num+w_num)>=5)){
                edge_point_b -= b_num*con_weight/10;
            }else{
                if(w_num<=0){
                    edge_point_b += b_num*con_weight/20;
                }
            }
        } */
        /* if(w_num<=0){
            edge_point_b += b_num*con_weight/10;
        }else{
            edge_point_b -= b_num*con_weight/10;
        } */
        //printf("\nnum:%d",b_num*con_weight*1/20);
        
    }
    //
    edge_point = edge_point_w - edge_point_b;
    //edge_point *= 0.3f;
    float pattern_point = (position_point(board_w) - position_point(board_b))*con_weight*0.9f;
    //printf("\nw:%f\nb:%f",edge_point_w,edge_point_b);
    
    //printf("\n%d,%d,%d,%d",legal_list_size_w , legal_list_size_b ,w_cx,b_cx);
    int white_score=0,black_score=0;
    eval_bitboard_score(board_w,board_b,&white_score,&black_score);
    float board_score=0;
    if(alpha!=1){
        board_score = white_score - black_score*1.1;
        dis_num = legal_list_size_w - legal_list_size_b -(w_cx-b_cx)*2;
    }
    
    float lose_keikoku = 0;
    //
    if(legal_list_size_b==0&&legal_list_size_w==0&&b_snum >= w_snum){
        lose_keikoku = -1000;
    }
    float pass_bonus = false_pass-true_pass;
    int board_weight,connect_weight,spread_weight;
    board_weight = 10;
    connect_weight = 10;
    spread_weight = 20;
    
    float score = (1-alpha) * (dis_num*2 +  board_score*board_weight + (touch_other*10+connected_pairs_score*connect_weight+open_stones_score*10 + spread_score*spread_weight)*(1-beta)) + alpha * (dis_num*0+(w_snum-b_snum)*10+(legal_list_size_w-legal_list_size_b)*10+connected_pairs_score*10);

    //


    //printf("\nscore:%f\ncon_score:%d\nedge_point:%f\ndis_num:%f\nalpha:%f\ndis:%d",board_score,con_score,edge_point,dis_num,alpha,w_snum-b_snum);
    //printf("\naa:%f",(score*10 + con_score*con_weight*10 + edge_point*10)/10);
    //printf("\naa:%d",zennmetu_keikoku + lose_keikoku + pass_bonus*90*alpha);
    //free(legal_list_w);
    //free(legal_list_b);
    //printf("\ndef_cx3");
    /* if(turn>40){
        con_score *=3;
    } */
    if (turn >= 20) {
        score += (pass_bonus) * 500.0f;
    }
    return ((float)(score + con_score*con_weight + edge_point)) + (float)zennmetu_keikoku + (float)lose_keikoku + pattern_point;
    //return (float)((score*10 + con_score*con_weight*10 + edge_point*10)/10) + (float)zennmetu_keikoku + (float)lose_keikoku + (float)(pass_bonus*90*alpha);
}



float evaluate_board(uint64_t board_w,uint64_t board_b,float true_pass,float false_pass){
    int con_weight = 100;
    uint64_t empty =  ~(board_w | board_b)& 0xFFFFFFFFFFFFFFFF;
    int turn = 64 - bit_count(empty);
    float alpha = nmin(nmax(0,(turn -35) / 20),1);

    uint64_t neighbor_mask_w =
    (board_w << 1 & RIGHT_MASK) |  // 左
    (board_w >> 1 & LEFT_MASK) |  // 右
    (board_w << 8) |                          // 上
    (board_w >> 8) |                         // 下
    (board_w << 7 & RIGHT_MASK) |  // 左上
    (board_w << 9 & LEFT_MASK) |  // 右上
    (board_w >> 7 & LEFT_MASK) |  // 右下
    (board_w >> 9 & RIGHT_MASK);   // 左下

    uint64_t neighbor_mask_b =
    (board_b << 1 & RIGHT_MASK) |  // 左
    (board_b >> 1 & LEFT_MASK) |  // 右
    (board_b << 8) |                          // 上
    (board_b >> 8) |                          // 下
    (board_b << 7 & RIGHT_MASK) |  // 左上
    (board_b << 9 & LEFT_MASK) |  // 右上
    (board_b >> 7 & LEFT_MASK) |  // 右下
    (board_b >> 9 & RIGHT_MASK);   // 左下

    uint64_t connected_pairs_w = neighbor_mask_w & board_w;
    uint64_t connected_pairs_b = neighbor_mask_b & board_b;

    //printf("\nconnect_w:%llu",connected_pairs_w);
    //printf("\nconnect_b:%llu",connected_pairs_b);
    
    uint64_t open_stones_w = neighbor_mask_w & empty;
    uint64_t open_stones_b = neighbor_mask_b & empty;

    //printf("\nopen_w:%llu",open_stones_w);
    //printf("\nopen_b:%llu",open_stones_b);

    float spread_w = calc_spread_penalty(board_w);
    float spread_b = calc_spread_penalty(board_b);

    float b_snum = bit_count(board_b);
    float w_snum = bit_count(board_w);

    float connected_pairs_w_num = bit_count(connected_pairs_w)/(w_snum+1);
    float connected_pairs_b_num = bit_count(connected_pairs_b)/(b_snum+1);

    float open_stones_w_num = bit_count(open_stones_w)/(w_snum+1);
    float open_stones_b_num = bit_count(open_stones_b)/(b_snum+1);

    float connected_pairs_score = connected_pairs_w_num - connected_pairs_b_num;
    float open_stones_score = -open_stones_w_num + open_stones_b_num;
    float spread_score = -spread_w + spread_b;


    RowCol legal_list_w[64];
    //RowCol *legal_list_w = malloc(sizeof(RowCol) * 64);
    //RowCol *legal_list_b = malloc(sizeof(RowCol) * 64);
    int legal_list_size_w=0;
    get_legal_square("white",board_w,board_b,legal_list_w,&legal_list_size_w);
    //printf("\nwhite_legal_finish");
    RowCol legal_list_b[64];
    int legal_list_size_b=0;
    //printf("\nblack_legal_start");
    get_legal_square("black",board_w,board_b,legal_list_b,&legal_list_size_b);
    //printf("\nlegal_list_size_b: %d\n", legal_list_size_b);
    //printf("\nblack_finish");
    //fflush(stdout);
    //printf("\ndef_cx");
    //fflush(stdout);
    int w_cx = 0,b_cx = 0,index=0;
    uint64_t w_legal=0,b_legal=0,legal_bit;
    //printf("\ndef_cx2");
    //fflush(stdout);
    for(int idx=0;idx<legal_list_size_w;idx++){
        RowCol legal = legal_list_w[idx];
        int row=legal.row,col=legal.col; 
        index = row*8 + col;
        legal_bit = ((uint64_t)1)  << index;
        w_legal |= legal_bit;
        for(int cx_idx=0;cx_idx<12;cx_idx++){
            RowCol cx = cx_zone[cx_idx];
            int cx_row = cx.row,cx_col = cx.col;
            if(cx_row == row && cx_col == col){
                w_cx++;
            }
        }
    }
    //fflush(stdout);
    for(int idx=0;idx<legal_list_size_b;idx++){
        RowCol legal = legal_list_b[idx];
        int row=legal.row,col=legal.col;
        index = row*8 + col;
        legal_bit = ((uint64_t)1)  << index;
        b_legal |= legal_bit;
        for(int cx_idx=0;cx_idx<12;cx_idx++){
            RowCol cx = cx_zone[cx_idx];
            int cx_row = cx.row,cx_col = cx.col;
            if(cx_row == row && cx_col == col){
                b_cx++;
            }
        }
    }
    //fflush(stdout);
    float zennmetu_keikoku = 0;
    if(bit_count(board_w) <= 2&&turn>=10){
        zennmetu_keikoku = -45;
    }
    //fflush(stdout);
    float edge_point_w = 0;
    float edge_point_b = 0;
    float edge_point,dis_num;
    uint64_t m_e_list[4] = {m1_e,m2_e,m3_e,m4_e};
    uint64_t m_c_list[4] = {m1_c,m2_c,m3_c,m4_c};
    uint64_t c_w=0,c_b=0;
    get_confirmed_stones(board_w,board_b,&c_w,&c_b);

    int c_w_num=bit_count(c_w);
    int c_b_num=bit_count(c_b);
    float con_score = (c_w_num - c_b_num);
    //fflush(stdout);
    for(int i=0;i<4;i++){
        int dec_point=0,add_point=0;

        int w_num=0,b_num=0;

        w_num = bit_count(board_w&m_e_list[i]);
        b_num = bit_count(board_b&m_e_list[i]);
        uint64_t w_corner = board_w&m_c_list[i];
        uint64_t b_corner = board_b&m_c_list[i];
        uint64_t w_corner_legal = w_legal&m_c_list[i];
        uint64_t b_corner_legal = b_legal&m_c_list[i];
        //printf("\nw:%d\nb:%d",w_num,b_num);
        if(w_num==6){
            int dec = 0;
            if(bit_count(b_corner)==1&&bit_count(w_corner)==0){
                dec_point += 10*6*con_weight* 4/5/10;
                dec = 1;
            }
            if(bit_count(b_corner_legal)>0){
                dec_point += 10*6*con_weight* 3/5/10;
                dec = 1;
            }
            if(dec==0){
                add_point += 10*6*con_weight* 4/5/10;
            }
        }
        //printf("\nadd:%d\ndec:%d",add_point,dec_point);
        edge_point_w += add_point;
        edge_point_w -= dec_point;
        dec_point = 0;
        add_point = 0;
        if(b_num == 0){
            edge_point_w += w_num*con_weight*1/20;
        }
        if(b_num==6){
            int dec = 0;
            if(bit_count(w_corner)==1&&bit_count(b_corner)==0){
                dec_point += 10*6*con_weight* 4/5/10;
                dec = 1;
                //printf("\n1:%f",10*6*con_weight* (4.0/5)/10);
            }
            if(bit_count(w_corner_legal)>0){
                dec_point += 10*6*con_weight* 3/5/10;
                dec = 1;
                //printf("\n2:%f",10*6*con_weight* (3.0/5)/10);
            }
            if(dec==0){
                add_point += 10*6*con_weight* 4/5/10;
                //printf("\n3:%f",10*6*con_weight* (4.0/5)/10);
            }
        }
        edge_point_b += add_point;
        edge_point_b -= dec_point;
        if (w_num==0){
            edge_point_b += b_num*con_weight*1/20;
            //printf("\nnum:%d",b_num*con_weight*1/20);
        }
    }
    //fflush(stdout);
    edge_point = edge_point_w - edge_point_b;
    //printf("\nw:%f\nb:%f",edge_point_w,edge_point_b);
    dis_num = legal_list_size_w - legal_list_size_b -w_cx+b_cx;
    //printf("\n%d,%d,%d,%d",legal_list_size_w , legal_list_size_b ,w_cx,b_cx);
    int white_score=0,black_score=0;
    eval_bitboard_score(board_w,board_b,&white_score,&black_score);

    float board_score = white_score - black_score*1.5;

    /* int b_snum = bit_count(board_b);
    int w_snum = bit_count(board_w); */
    float lose_keikoku = 0;
    //fflush(stdout);
    if(legal_list_size_b==0&&legal_list_size_w==0&&b_snum > w_snum){
        lose_keikoku = -1000;
    }
    float pass_bonus = false_pass-true_pass;

    float score = (1-alpha) * (dis_num*300/100 +  board_score*200/100) + alpha * ((w_snum-b_snum)*5000/100+(legal_list_size_w-legal_list_size_b)*400/100);

    //fflush(stdout);


    //printf("\nscore:%f\ncon_score:%d\nedge_point:%f\ndis_num:%f\nalpha:%f\ndis:%d",board_score,con_score,edge_point,dis_num,alpha,w_snum-b_snum);
    //printf("\naa:%f",(score*10 + con_score*con_weight*10 + edge_point*10)/10);
    //printf("\naa:%d",zennmetu_keikoku + lose_keikoku + pass_bonus*90*alpha);
    //free(legal_list_w);
    //free(legal_list_b);
    //printf("\ndef_cx3");
    return ((float)(score*10 + con_score*con_weight*10 + edge_point*10) / 10.0f) + (float)zennmetu_keikoku + (float)lose_keikoku + (float)(pass_bonus*90.0f*alpha);
    //return (float)((score*10 + con_score*con_weight*10 + edge_point*10)/10) + (float)zennmetu_keikoku + (float)lose_keikoku + (float)(pass_bonus*90*alpha);
}

float evaluate_board2(uint64_t board_w,uint64_t board_b,float true_pass,float false_pass,
    float con_weight,
    float edge_weight,
    float dis_num_weight,
    float board_weight,
    float dis_sum_weight,
    float legal_dis_weight,
    float connect_weight,
    float spread_weight,
    float open_weight,
    float alpha_start,
    float alpha_speed,
    float beta_start,
    float beta_speed,
    float pass_weight
    ){
    //int con_weight = 100;
    uint64_t empty =  ~(board_w | board_b)& 0xFFFFFFFFFFFFFFFF;
    int turn = 64 - bit_count(empty);
    float alpha = nmin(nmax(0,(turn -alpha_start) / alpha_speed),1);
    float beta = nmin(nmax(0,(turn -beta_start) / beta_speed),1);


    float spread_score = 0,open_stones_score = 0,connected_pairs_score = 0;
    float b_snum = bit_count(board_b);
    float w_snum = bit_count(board_w);
    float connect_w_num,connect_b_num,open_w_num,open_b_num,touch_w2b,touch_b2w,touch_other;
    find_connected_open_stone2(board_w,board_b,empty,&connect_w_num,&open_w_num,&touch_w2b);
    find_connected_open_stone2(board_b,board_w,empty,&connect_b_num,&open_b_num,&touch_b2w);
    if(beta!=1){
        

        
        

        float spread_w = calc_spread_penalty(board_w);
        float spread_b = calc_spread_penalty(board_b);

        

        

        float open_stones_w_num = open_w_num/(w_snum+0.1);
        float open_stones_b_num = open_b_num/(b_snum+0.1);

        
        open_stones_score = -open_stones_w_num + open_stones_b_num;
        spread_score = -spread_w + spread_b;
    }
    touch_other = touch_w2b/(w_snum+0.1) - touch_b2w/(b_snum+0.1);
    float connected_pairs_w_num = connect_w_num/(w_snum+0.1);
    float connected_pairs_b_num = connect_b_num/(b_snum+0.1);
    connected_pairs_score = connected_pairs_w_num - connected_pairs_b_num;
    
    //printf("\nopen_w:%f",open_stones_w_num);
    //printf("\nopen_b:%f",open_stones_b_num); 
    //printf("\nconect_w:%f",connected_pairs_w_num);
    //printf("\nconect_b:%f",connected_pairs_b_num);

    RowCol legal_list_w[64];
    //RowCol *legal_list_w = malloc(sizeof(RowCol) * 64);
    //RowCol *legal_list_b = malloc(sizeof(RowCol) * 64);
    int legal_list_size_w=0;
    get_legal_square("white",board_w,board_b,legal_list_w,&legal_list_size_w);
    //printf("\nwhite_legal_finish");
    RowCol legal_list_b[64];
    int legal_list_size_b=0;
    //printf("\nblack_legal_start");
    get_legal_square("black",board_w,board_b,legal_list_b,&legal_list_size_b);
    //printf("\nlegal_list_size_b: %d\n", legal_list_size_b);
    //printf("\nblack_finish");
    //fflush(stdout);
    //printf("\ndef_cx");
    //fflush(stdout);
    int w_cx = 0,b_cx = 0,index=0;
    uint64_t w_legal=0,b_legal=0,legal_bit;
    //printf("\ndef_cx2");
    //fflush(stdout);
    for(int idx=0;idx<legal_list_size_w;idx++){
        RowCol legal = legal_list_w[idx];
        int row=legal.row,col=legal.col; 
        index = row*8 + col;
        legal_bit = ((uint64_t)1)  << index;
        w_legal |= legal_bit;
        for(int cx_idx=0;cx_idx<12;cx_idx++){
            RowCol cx = cx_zone[cx_idx];
            int cx_row = cx.row,cx_col = cx.col;
            if(cx_row == row && cx_col == col){
                w_cx++;
            }
        }
    }
    //fflush(stdout);
    for(int idx=0;idx<legal_list_size_b;idx++){
        RowCol legal = legal_list_b[idx];
        int row=legal.row,col=legal.col;
        index = row*8 + col;
        legal_bit = ((uint64_t)1)  << index;
        b_legal |= legal_bit;
        for(int cx_idx=0;cx_idx<12;cx_idx++){
            RowCol cx = cx_zone[cx_idx];
            int cx_row = cx.row,cx_col = cx.col;
            if(cx_row == row && cx_col == col){
                b_cx++;
            }
        }
    }
    //fflush(stdout);
    float zennmetu_keikoku = 0;
    if(bit_count(board_w) <= 2&&turn>=10){
        zennmetu_keikoku = -6000;
    }
    //fflush(stdout);
    float edge_point_w = 0;
    float edge_point_b = 0;
    float edge_point,dis_num;
    uint64_t m_e_list[4] = {m1_e,m2_e,m3_e,m4_e};
    uint64_t m_c_list[4] = {m1_c,m2_c,m3_c,m4_c};
    uint64_t c_w=0,c_b=0;
    get_confirmed_stones(board_w,board_b,&c_w,&c_b);

    int c_w_num=bit_count(c_w);
    int c_b_num=bit_count(c_b);
    float con_score = (c_w_num - c_b_num);
    //fflush(stdout);
    for(int i=0;i<4;i++){
        int dec_point=0,add_point=0;

        int w_num=0,b_num=0;

        w_num = bit_count(board_w&m_e_list[i]);
        b_num = bit_count(board_b&m_e_list[i]);
        uint64_t w_corner = board_w&m_c_list[i];
        uint64_t b_corner = board_b&m_c_list[i];
        uint64_t w_corner_legal = w_legal&m_c_list[i];
        uint64_t b_corner_legal = b_legal&m_c_list[i];
        int b_c_num = bit_count(b_corner);
        int w_c_num = bit_count(w_corner);
        int w_c_legal_num = bit_count(w_corner_legal);
        int b_c_legal_num = bit_count(b_corner_legal);
        int edge_con_num_w = bit_count(m_e_list[i]&c_w);
        int edge_con_num_b = bit_count(m_e_list[i]&c_b);
        //printf("\nw:%d\nb:%d",w_num,b_num);
        if(w_num==6){
            int dec = 0;
            if(b_c_num==1&&w_c_num==0){
                dec_point += 6*con_weight*0.9;
                dec = 1;
            }
            if(b_c_legal_num>0){
                dec_point += 6*con_weight* 0.8;
                dec = 1;
            }
            if(b_c_num==0){
                add_point += 6*con_weight* 0.3;
            }
        }
        
        if((w_num+b_num)==6&&b_num!=0){
            if(w_c_num<=1){
                dec_point += (6-edge_con_num_w)*con_weight*0.9;
            }
            if((edge_con_num_w+edge_con_num_b)<=5&&w_c_num==1&&w_c_legal_num==1){
                dec_point += (b_num-edge_con_num_b)*con_weight*0.9;
            }
        }
        //printf("\nadd:%d\ndec:%d",add_point,dec_point);
        edge_point_w += add_point;// + w_c_legal_num*6*0.4;
        edge_point_w -= dec_point;
        dec_point = 0;
        add_point = 0;
        /* if(b_num<=3){
            if(b_c_legal_num>=1&&(b_num+w_num)>=5){
                edge_point_w -= w_num*con_weight/10;
            }else{
                if(b_num<=0){
                    edge_point_w += w_num*con_weight/20;
                }
            }
        } */
        /* if(b_num<=0){
            edge_point_w += w_num*con_weight/10;
        }else{
            edge_point_w -= w_num*con_weight/10;
        } */
        if(b_num==6){
            int dec = 0;
            if(w_c_num==1&&b_c_num==0){
                dec_point += 6*con_weight*0.9;
                dec = 1;
                //printf("\n1:%f",10*6*con_weight* (4.0/5)/10);
            }
            if(w_c_legal_num>0){
                dec_point += 6*con_weight* 0.8;
                dec = 1;
                //printf("\n2:%f",10*6*con_weight* (3.0/5)/10);
            }
            if(dec==0){
                add_point += 6*con_weight* 0.3;
                //printf("\n3:%f",10*6*con_weight* (4.0/5)/10);
            }
        }
        if((w_num+b_num)==6&&w_num!=0){
            if(b_c_num<=1){
                dec_point += (6-edge_con_num_b)*con_weight*0.9;
            }
            if((edge_con_num_b+edge_con_num_w)<=5&&b_c_num==1&&b_c_legal_num==1){
                dec_point += (w_num-edge_con_num_w)*con_weight*0.9;
            }
        }
        edge_point_b += add_point;
        edge_point_b -= dec_point;
        /* if (w_num==0){
            edge_point_b += b_num*con_weight*1/20;
            //printf("\nnum:%d",b_num*con_weight*1/20);
        } */
    }
    //fflush(stdout);
    edge_point = edge_point_w - edge_point_b;
    edge_point *= edge_weight;
    //printf("\nw:%f\nb:%f",edge_point_w,edge_point_b);
    dis_num = legal_list_size_w - legal_list_size_b -w_cx+b_cx;
    //printf("\n%d,%d,%d,%d",legal_list_size_w , legal_list_size_b ,w_cx,b_cx);
    int white_score=0,black_score=0;
    eval_bitboard_score(board_w,board_b,&white_score,&black_score);

    float board_score = white_score - black_score*1.5;

    /* int b_snum = bit_count(board_b);
    int w_snum = bit_count(board_w); */
    float lose_keikoku = 0;
    //fflush(stdout);
    if(legal_list_size_b==0&&legal_list_size_w==0&&b_snum > w_snum){
        lose_keikoku = -1000;
    }
    float pass_bonus = false_pass-true_pass;

    float score = (1-alpha) * (dis_num*dis_num_weight +  board_score*board_weight + (connected_pairs_score*connect_weight + open_stones_score*open_weight + spread_score*spread_weight)*(1-beta)) + alpha * ((w_snum-b_snum)*dis_sum_weight+(legal_list_size_w-legal_list_size_b)*legal_dis_weight);

    //fflush(stdout);


    //printf("\nscore:%f\ncon_score:%d\nedge_point:%f\ndis_num:%f\nalpha:%f\ndis:%d",board_score,con_score,edge_point,dis_num,alpha,w_snum-b_snum);
    //printf("\naa:%f",(score*10 + con_score*con_weight*10 + edge_point*10)/10);
    //printf("\naa:%d",zennmetu_keikoku + lose_keikoku + pass_bonus*90*alpha);
    //free(legal_list_w);
    //free(legal_list_b);
    //printf("\ndef_cx3");
    if (turn >= 30) {
        score += (pass_bonus) * pass_weight;
    }
    return ((float)(score + con_score*con_weight + edge_point)) + (float)zennmetu_keikoku + (float)lose_keikoku;
    //return (float)((score*10 + con_score*con_weight*10 + edge_point*10)/10) + (float)zennmetu_keikoku + (float)lose_keikoku + (float)(pass_bonus*90*alpha);
}
float evaluate_board3(uint64_t board_w,uint64_t board_b,float true_pass,float false_pass,
    float con_weight,
    float edge_weight,
    float dis_num_weight,
    float board_weight,
    float dis_sum_weight,
    float legal_dis_weight,
    float connect_weight,
    float spread_weight,
    float open_weight,
    float alpha_start,
    float alpha_speed,
    float beta_start,
    float beta_speed,
    float pass_weight,
    float touch_weight,
    float pattern_weight,
    float connect_weight2,
    float cx_point_weight,
    float corner_point_weight
    ){
    //con_weight = 100;
    uint64_t empty =  ~(board_w | board_b)& 0xFFFFFFFFFFFFFFFF;
    int turn = 64 - bit_count(empty);
    float alpha = nmin(nmax(0,(turn -alpha_start) / alpha_speed),1);
    float beta = nmin(nmax(0,(turn -beta_start) / beta_speed),1);

    float spread_score = 0,open_stones_score = 0,connected_pairs_score = 0;
    float b_snum = bit_count(board_b);
    float w_snum = bit_count(board_w);
    float connect_w_num,connect_b_num,open_w_num,open_b_num,touch_w2b,touch_b2w,touch_other;
    find_connected_open_stone2(board_w,board_b,empty,&connect_w_num,&open_w_num,&touch_w2b);
    find_connected_open_stone2(board_b,board_w,empty,&connect_b_num,&open_b_num,&touch_b2w);
    if(beta!=1){
        

        
        

        float spread_w = calc_spread_penalty(board_w);
        float spread_b = calc_spread_penalty(board_b);

        

        float open_stones_w_num,touch_w2b_pone,touch_b2w_pone,connected_pairs_w_num,connected_pairs_b_num;
        if(w_snum==0){
            open_stones_w_num = 0;
            touch_w2b_pone = 0;
            connected_pairs_w_num = 0;
        }else{
            open_stones_w_num = open_w_num/w_snum;
            touch_w2b_pone = touch_w2b/w_snum;
            connected_pairs_w_num = connect_w_num/w_snum;
        }
        float open_stones_b_num;
        if(b_snum==0){
            open_stones_b_num = 0;
            touch_b2w_pone = 0;
            connected_pairs_b_num = 0;
        }else{
            open_stones_b_num = open_b_num/b_snum;
            touch_b2w_pone = touch_b2w/b_snum;
            connected_pairs_b_num = connect_b_num/b_snum;
        }

        
        open_stones_score = -open_stones_w_num + open_stones_b_num;
        spread_score = -spread_w + spread_b;
        touch_other = touch_w2b_pone - touch_b2w_pone;
        connected_pairs_score = connected_pairs_w_num - connected_pairs_b_num;
    }
    

    
    RowCol legal_list_w[64];
    //RowCol *legal_list_w = malloc(sizeof(RowCol) * 64);
    //RowCol *legal_list_b = malloc(sizeof(RowCol) * 64);
    int legal_list_size_w=0;
    get_legal_square("white",board_w,board_b,legal_list_w,&legal_list_size_w);
    //printf("\nwhite_legal_finish");
    RowCol legal_list_b[64];
    int legal_list_size_b=0;
    //printf("\nblack_legal_start");
    get_legal_square("black",board_w,board_b,legal_list_b,&legal_list_size_b);
    //printf("\nlegal_list_size_b: %d\n", legal_list_size_b);
    //printf("\nblack_finish");
    //fflush(stdout);
    //printf("\ndef_cx");
    //fflush(stdout);
    int w_cx = 0,b_cx = 0,index=0;
    uint64_t w_legal=0,b_legal=0,legal_bit;
    //printf("\ndef_cx2");
    //fflush(stdout);
    for(int idx=0;idx<legal_list_size_w;idx++){
        RowCol legal = legal_list_w[idx];
        int row=legal.row,col=legal.col; 
        index = row*8 + col;
        legal_bit = ((uint64_t)1)  << index;
        w_legal |= legal_bit;
        for(int cx_idx=0;cx_idx<12;cx_idx++){
            RowCol cx = cx_zone[cx_idx];
            int cx_row = cx.row,cx_col = cx.col;
            if(cx_row == row && cx_col == col){
                w_cx++;
            }
        }
    }
    //fflush(stdout);
    for(int idx=0;idx<legal_list_size_b;idx++){
        RowCol legal = legal_list_b[idx];
        int row=legal.row,col=legal.col;
        index = row*8 + col;
        legal_bit = ((uint64_t)1)  << index;
        b_legal |= legal_bit;
        for(int cx_idx=0;cx_idx<12;cx_idx++){
            RowCol cx = cx_zone[cx_idx];
            int cx_row = cx.row,cx_col = cx.col;
            if(cx_row == row && cx_col == col){
                b_cx++;
            }
        }
    }
    //fflush(stdout);
    float zennmetu_keikoku = 0;
    if(bit_count(board_w) <= 2&&turn>=10){
        zennmetu_keikoku = -4500;
    }
    //fflush(stdout);
    float edge_point_w = 0;
    float edge_point_b = 0;
    float edge_point,dis_num;
    uint64_t m_e_list[4] = {m1_e,m2_e,m3_e,m4_e};
    uint64_t m_c_list[4] = {m1_c,m2_c,m3_c,m4_c};
    uint64_t c_w=0,c_b=0;
    if(turn>=30){
        get_confirmed_stones(board_w,board_b,&c_w,&c_b);
    }else{
        get_confirmed_stones_lite(board_w,board_b,&c_w,&c_b);
    }
    
    int c_w_num=bit_count(c_w);
    int c_b_num=bit_count(c_b);
    float con_score = (c_w_num - c_b_num);
    //fflush(stdout);
    int w_corner_num=0,b_corner_num=0;
    for(int i=0;i<4;i++){
        int dec_point=0,add_point=0;

        int w_num=0,b_num=0;

        w_num = bit_count(board_w&m_e_list[i]);
        b_num = bit_count(board_b&m_e_list[i]);
        
        uint64_t w_corner = board_w&m_c_list[i];
        uint64_t b_corner = board_b&m_c_list[i];
        uint64_t w_corner_legal = w_legal&m_c_list[i];
        uint64_t b_corner_legal = b_legal&m_c_list[i];
        int b_c_num = bit_count(b_corner);
        int w_c_num = bit_count(w_corner);
        w_corner_num += w_c_num;
        b_corner_num += b_c_num;
        //printf("\nw:%d\nb:%d",w_num,b_num);
        if(w_num==6){
            int dec = 0;
            if(b_c_num==1&&w_c_num==0){
                dec_point += 10*6*con_weight* 4/5/10;
                dec = 1;
            }
            if(bit_count(b_corner_legal)>0){
                dec_point += 10*6*con_weight* 3/5/10;
                dec = 1;
            }
            if(dec==0){
                add_point += 10*6*con_weight* 4/5/10;
            }
        }
        for(int k=0;k<8;k++){
            if((((m_e_list[i]|m_c_list[i])&board_w)&edge_pattern[k])==edge_pattern[k]&&w_num==5&&w_c_num==1&&b_c_num==0){
                dec_point +=8*con_weight*0.9f;
            }
        }
        //printf("\nadd:%d\ndec:%d",add_point,dec_point);
        edge_point_w += add_point;
        edge_point_w -= dec_point;
        dec_point = 0;
        add_point = 0;
        if(b_num == 0&&w_c_num==0&&b_c_num==0){
            edge_point_w += w_num*con_weight*0.2f;
        }else if(w_num<=6&&b_c_num==1&&w_c_num==0){
            edge_point_w -= w_num*con_weight*0.9f;
        }
        if(b_num==6){
            int dec = 0;
            if(w_c_num==1&&b_c_num==0){
                dec_point += 10*6*con_weight* 4/5/10;
                dec = 1;
                //printf("\n1:%f",10*6*con_weight* (4.0/5)/10);
            }
            if(bit_count(w_corner_legal)>0){
                dec_point += 10*6*con_weight* 3/5/10;
                dec = 1;
                //printf("\n2:%f",10*6*con_weight* (3.0/5)/10);
            }
            if(dec==0){
                add_point += 10*6*con_weight* 4/5/10;
                //printf("\n3:%f",10*6*con_weight* (4.0/5)/10);
            }
        }
        for(int k=0;k<8;k++){
            if((((m_e_list[i]|m_c_list[i])&board_b)&edge_pattern[k])==edge_pattern[k]&&b_num==5&&b_c_num==1&&w_c_num==0){
                dec_point += 8*con_weight*0.9f;
            }
        }
        edge_point_b += add_point;
        edge_point_b -= dec_point;
        if (w_num==0&&w_c_num==0&&b_c_num==0){
            edge_point_b += b_num*con_weight*0.2f;
            //printf("\nnum:%d",b_num*con_weight*1/20);
        }else if(b_num<=6&&w_c_num==1&&b_c_num==0){
            edge_point_b -= b_num*con_weight*0.9f;
        }
    }
    //fflush(stdout);
    edge_point = edge_point_w - edge_point_b;
    //printf("\nw:%f\nb:%f",edge_point_w,edge_point_b);
    dis_num = legal_list_size_w - legal_list_size_b -(w_cx-b_cx)*cx_point_weight+(w_corner_num-b_corner_num)*corner_point_weight;
    //printf("\n%d,%d,%d,%d",legal_list_size_w , legal_list_size_b ,w_cx,b_cx);
    int white_score=0,black_score=0;
    eval_bitboard_score(board_w,board_b,&white_score,&black_score);

    float board_score = white_score - black_score*1.5f;
    float pattern_point = 0;
    pattern_point = (position_point(board_w) - position_point(board_b))*con_weight*pattern_weight;
    /* int b_snum = bit_count(board_b);
    int w_snum = bit_count(board_w); */
    float lose_keikoku = 0;
    //fflush(stdout);
    if(legal_list_size_b==0&&legal_list_size_w==0&&b_snum > w_snum){
        lose_keikoku = -1000;
    }
    float pass_bonus = false_pass-true_pass;
    float score = (1-alpha) * (dis_num*dis_num_weight + board_score*board_weight + (touch_other*touch_weight+connected_pairs_score*connect_weight+open_stones_score*open_weight + spread_score*spread_weight)*(1-beta)) + alpha * ((w_snum-b_snum)*con_weight*dis_sum_weight+(legal_list_size_w-legal_list_size_b)*legal_dis_weight+connected_pairs_score*connect_weight2);
    //float score = (1-alpha) * (dis_num*300/100 +  board_score*200/100) + alpha * ((w_snum-b_snum)*5000/100+(legal_list_size_w-legal_list_size_b)*400/100);

    //fflush(stdout);


    //printf("\nscore:%f\ncon_score:%d\nedge_point:%f\ndis_num:%f\nalpha:%f\ndis:%d",board_score,con_score,edge_point,dis_num,alpha,w_snum-b_snum);
    //printf("\naa:%f",(score*10 + con_score*con_weight*10 + edge_point*10)/10);
    //printf("\naa:%d",zennmetu_keikoku + lose_keikoku + pass_bonus*90*alpha);
    //free(legal_list_w);
    //free(legal_list_b);
    //printf("\ndef_cx3");
    printf("score:%2.f\n",score);
    printf("edge:%2.f\n",edge_point);
    printf("stable:%2.f\n",con_score);
    printf("zennmetu_keikoku:%2.f\n",(float)zennmetu_keikoku);
    printf("pass:%2.f\n",(float)pass_bonus);
    printf("pattern:%2.f\n",(float)pattern_point);
    return ((float)(score + con_score*con_weight + edge_point*edge_weight)) + (float)zennmetu_keikoku + (float)lose_keikoku + (float)(pass_bonus*pass_weight) + pattern_point;
    //return (float)((score*10 + con_score*con_weight*10 + edge_point*10)/10) + (float)zennmetu_keikoku + (float)lose_keikoku + (float)(pass_bonus*90*alpha);
}

int is_terminal(uint64_t board_w,uint64_t board_b){
    //if get_legal_square("white",board) == [] and get_legal_square("black",board.copy()*-1) == []:
    if (bit_count(board_w | board_b) == 64){
        return TRUE;
    }else{
        return FALSE;
    }
}
int compare_move_order_desc(const void *a, const void *b) {
    MoveOrder *orderA = (MoveOrder *)a;
    MoveOrder *orderB = (MoveOrder *)b;

    if (orderA->score < orderB->score) return 1;
    if (orderA->score > orderB->score) return -1;
    return 0;
}
int compare_move_order_asc(const void *a, const void *b) {
    MoveOrder *orderA = (MoveOrder *)a;
    MoveOrder *orderB = (MoveOrder *)b;

    if (orderA->score < orderB->score) return -1;
    if (orderA->score > orderB->score) return 1;
    return 0;
}
static int call_count = 0;

void minimax(uint64_t board_w,uint64_t board_b,int depth,float alpha,float beta,int maximizing_player,int true_pass,int false_pass,RowCol *act,float *score){
    /* call_count++;
    printf("call #%d, depth=%d\n", call_count, depth);
    printf("board_w: 0x%016llX\n", board_w);
    printf("board_b: 0x%016llX\n", board_b); */
    //fflush(stdout);
    //printf("%d,%d\n",call_count,depth);
    /* if (call_count > 600) {
        
        printf("Recursion limit exceeded\n");
        return;
    } */
    //printf("=== minimax() 入った ===\n"); fflush(stdout);
    //printf("\nBoard Wm: %llu\n", board_w);
    //printf("order1\n");
    //printf("Board Bm: %llu\n", board_b);
    //printf("depth:%d\n",depth);
    //printf("order2\n");
    //printf("depth:%d",depth);
    //RowCol *legal_list_w = malloc(sizeof(RowCol) * 64);
    //RowCol *legal_list_b = malloc(sizeof(RowCol) * 64);
    fflush(stdout);
    RowCol legal_list_w[64];
    int legal_list_size_w = 0;
    get_legal_square("white",board_w,board_b,legal_list_w,&legal_list_size_w);
    //printf("\norder1\n");
    RowCol legal_list_b[64];
    int legal_list_size_b = 0;
    //printf("\norder2\n");
    get_legal_square("black",board_w,board_b,legal_list_b,&legal_list_size_b);
    //printf("\norder3");
    RowCol result_act = *act;
    float result_score = *score;
    if (depth == 0 || is_terminal(board_w,board_b) || (legal_list_size_w==0 && legal_list_size_b==0)){
        //printf("\n葉ノード");
        //printf("\nt:%d",true_pass);
        //printf("\nf:%d",false_pass);
        *score = evaluate_board(board_w,board_b,true_pass,false_pass);
        //fflush(stdout);
        //printf("\nscore:%f\n",*score);
        //free(legal_list_w);
        //free(legal_list_b);
        return;
    }
    
    if (maximizing_player){
        RowCol* legal_list = legal_list_w;
        int legal_size = legal_list_size_w;
        if(legal_size==0){
            minimax(board_w,board_b,depth,alpha,beta,FALSE,true_pass+1,false_pass,&*act,&*score);
            return;
        }
        float max_eval = -INFINITY;
        RowCol best_move;
        
        //RowCol move_oder_list[64];
        int move_oder_list_size = legal_size;
        MoveOrder move_order_list[64];
        //MoveOrder *move_order_list = malloc(sizeof(MoveOrder) * move_oder_list_size);
        for(int i=0;i<legal_size;i++){
            RowCol move = legal_list[i];
            RowCol flip_list[64];
            int flip_list_size;
            uint64_t new_board_w = board_w,new_board_b = board_b;
            
            fflush(stdout);
            identify_flip_stone("white",&new_board_w,&new_board_b,move,1,flip_list,&flip_list_size);
            RowCol result_act2;
            float result_score2;
            /* printf(">>> call:%d",call_count);
            printf(">>> Trying move (%d, %d) at depth=%d\n", move.row, move.col, depth);
            printf(">>> New W: 0x%016llX\n", new_board_w);
            printf(">>> New B: 0x%016llX\n", new_board_b); */
            minimax(new_board_w,new_board_b,0,alpha,beta,FALSE,true_pass,false_pass,&result_act2,&result_score2);
            move_order_list[i] = (MoveOrder){result_score2,move,new_board_w,new_board_b};
        }
        qsort(move_order_list, move_oder_list_size, sizeof(MoveOrder), compare_move_order_desc);//maximizing_player＝=Falseでは昇順に
        for(int i=0;i<legal_size;i++){
            RowCol move = move_order_list[i].move;
            uint64_t new_board_w = move_order_list[i].newBoardW;
            uint64_t new_board_b = move_order_list[i].newBoardB;

            float eval;
            RowCol act2;

            minimax(new_board_w,new_board_b,depth-1,alpha,beta,FALSE,true_pass,false_pass,&act2,&eval);
            if (eval > max_eval){
                max_eval = eval;
                best_move = move;
            }
            alpha = nmax(alpha, eval);
            if (beta <= alpha){
                break;
            }
        }
        //printf(">>> return action: (%d,%d)\n", best_move.row, best_move.col);
        *act = best_move;
        *score = max_eval;
        //free(move_order_list);
        //free(legal_list_w);
        //free(legal_list_b);
        return;
    }else{
        RowCol* legal_list = legal_list_b;
        int legal_size = legal_list_size_b;
        if(legal_size==0){
            minimax(board_w,board_b,depth,alpha,beta,TRUE,true_pass,false_pass+1,&*act,&*score);
            return;
        }
        float min_eval = INFINITY;
        RowCol best_move;

        int move_oder_list_size = legal_size;
        MoveOrder move_order_list[64];
        //MoveOrder *move_order_list = malloc(sizeof(MoveOrder) * move_oder_list_size);
        for(int i=0;i<legal_size;i++){
            RowCol move = legal_list[i];
            RowCol flip_list[64];
            int flip_list_size;
            uint64_t new_board_w = board_w,new_board_b = board_b;
            identify_flip_stone("black",&new_board_w,&new_board_b,move,1,flip_list,&flip_list_size);
            RowCol result_act2;
            float result_score2;
            minimax(new_board_w,new_board_b,0,alpha,beta,TRUE,true_pass,false_pass,&result_act2,&result_score2);
            move_order_list[i] = (MoveOrder){result_score2,move,new_board_w,new_board_b};
        }
        qsort(move_order_list, move_oder_list_size, sizeof(MoveOrder), compare_move_order_asc);
        for(int i=0;i<legal_size;i++){
            RowCol move = move_order_list[i].move;
            uint64_t new_board_w = move_order_list[i].newBoardW;
            uint64_t new_board_b = move_order_list[i].newBoardB;

            float eval;
            RowCol act2;

            minimax(new_board_w,new_board_b,depth-1,alpha,beta,TRUE,true_pass,false_pass,&act2,&eval);
            //printf("\nact:(%d,%d),score:%f",move.row,move.col,eval);
            if (eval < min_eval){
                min_eval = eval;
                best_move = move;
            }
            beta = nmin(beta, eval);
            if (beta <= alpha){
                break;
            }
        }
        *act = best_move;
        *score = min_eval;
        //free(move_order_list);
        //free(legal_list_w);
        //free(legal_list_b);
        //printf(">>> return action: (%d,%d)\n", best_move.row, best_move.col);
        return;
    }
}

void minimax_split(uint32_t board_w_high, uint32_t board_w_low,
                    uint32_t board_b_high, uint32_t board_b_low,
                    int depth, float alpha, float beta, int maximizing_player,
                    int true_pass, int false_pass, RowCol *act, float *score) {
    uint64_t board_w = ((uint64_t)board_w_high << 32) | board_w_low;
    uint64_t board_b = ((uint64_t)board_b_high << 32) | board_b_low;
    printf("\nBoard W: 0x%016llX\n", board_w);
    printf("Board B: 0x%016llX\n", board_b);
    minimax(board_w, board_b, depth, alpha, beta, maximizing_player, true_pass, false_pass, &*act, &*score);
}

void minimax2(uint64_t board_w,uint64_t board_b,int depth,float alpha,float beta,int maximizing_player,int true_pass,int false_pass,RowCol *act,float *score){
    /* call_count++;
    printf("call #%d, depth=%d\n", call_count, depth);
    printf("board_w: 0x%016llX\n", board_w);
    printf("board_b: 0x%016llX\n", board_b); */
    fflush(stdout);
    const char* shm_name = "my_shm";
    HANDLE hMapFile = OpenFileMappingA(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        shm_name);
    int32_t* pBuf = (int32_t*) MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if(pBuf[0]!=123){
        return;
    }
    //printf("%d,%d\n",call_count,depth);
    /* if (call_count > 600) {
        
        printf("Recursion limit exceeded\n");
        return;
    } */
    //printf("=== minimax() 入った ===\n"); fflush(stdout);
    //printf("\nBoard Wm: %llu\n", board_w);
    //printf("order1\n");
    //printf("Board Bm: %llu\n", board_b);
    //printf("depth:%d\n",depth);
    //printf("order2\n");
    //printf("depth:%d",depth);
    RowCol *legal_list_w = malloc(sizeof(RowCol) * 64);
    RowCol *legal_list_b = malloc(sizeof(RowCol) * 64);
    fflush(stdout);
    //RowCol legal_list_w[64];
    int legal_list_size_w = 0;
    get_legal_square("white",board_w,board_b,legal_list_w,&legal_list_size_w);
    //printf("\norder1\n");
    //RowCol legal_list_b[64];
    int legal_list_size_b = 0;
    //printf("\norder2\n");
    get_legal_square("black",board_w,board_b,legal_list_b,&legal_list_size_b);
    //printf("\norder3");
    RowCol result_act = *act;
    float result_score = *score;
    if (depth == 0 || is_terminal(board_w,board_b) || (legal_list_size_w==0 && legal_list_size_b==0)){
        //printf("\n葉ノード");
        //printf("\nt:%d",true_pass);
        //printf("\nf:%d",false_pass);
        *score = evaluate_board(board_w,board_b,true_pass,false_pass);
        fflush(stdout);
        //printf("\nscore:%f\n",*score);
        free(legal_list_w);
        free(legal_list_b);
        return;
    }
    
    if (maximizing_player){
        RowCol* legal_list = legal_list_w;
        int legal_size = legal_list_size_w;
        if(legal_size==0){
            minimax2(board_w,board_b,depth,alpha,beta,FALSE,true_pass+1,false_pass,&result_act,&result_score);
        }
        float max_eval = -INFINITY;
        RowCol best_move;
        
        //RowCol move_oder_list[64];
        int move_oder_list_size = legal_size;
        //MoveOrder move_order_list[64];
        MoveOrder *move_order_list = malloc(sizeof(MoveOrder) * move_oder_list_size);
        for(int i=0;i<legal_size;i++){
            RowCol move = legal_list[i];
            RowCol flip_list[64];
            int flip_list_size;
            uint64_t new_board_w = board_w,new_board_b = board_b;
            
            fflush(stdout);
            identify_flip_stone("white",&new_board_w,&new_board_b,move,1,flip_list,&flip_list_size);
            RowCol result_act2;
            float result_score2;
            /* printf(">>> call:%d",call_count);
            printf(">>> Trying move (%d, %d) at depth=%d\n", move.row, move.col, depth);
            printf(">>> New W: 0x%016llX\n", new_board_w);
            printf(">>> New B: 0x%016llX\n", new_board_b); */
            minimax2(new_board_w,new_board_b,0,alpha,beta,FALSE,true_pass,false_pass,&result_act2,&result_score2);
            move_order_list[i] = (MoveOrder){result_score2,move,new_board_w,new_board_b};
        }
        qsort(move_order_list, move_oder_list_size, sizeof(MoveOrder), compare_move_order_desc);//maximizing_player＝=Falseでは昇順に
        for(int i=0;i<legal_size;i++){
            RowCol move = move_order_list[i].move;
            uint64_t new_board_w = move_order_list[i].newBoardW;
            uint64_t new_board_b = move_order_list[i].newBoardB;

            float eval;
            RowCol act2;

            minimax2(new_board_w,new_board_b,depth-1,alpha,beta,FALSE,true_pass,false_pass,&act2,&eval);
            if (eval > max_eval){
                max_eval = eval;
                best_move = move;
            }
            alpha = nmax(alpha, eval);
            if (beta <= alpha){
                break;
            }
        }
        *act = best_move;
        *score = max_eval;
        free(move_order_list);
        free(legal_list_w);
        free(legal_list_b);
        return;
    }else{
        RowCol* legal_list = legal_list_b;
        int legal_size = legal_list_size_b;
        if(legal_size==0){
            minimax2(board_w,board_b,depth,alpha,beta,TRUE,true_pass,false_pass+1,&result_act,&result_score);
        }
        float min_eval = INFINITY;
        RowCol best_move;

        int move_oder_list_size = legal_size;
        //MoveOrder move_order_list[64];
        MoveOrder *move_order_list = malloc(sizeof(MoveOrder) * move_oder_list_size);
        for(int i=0;i<legal_size;i++){
            RowCol move = legal_list[i];
            RowCol flip_list[64];
            int flip_list_size;
            uint64_t new_board_w = board_w,new_board_b = board_b;
            identify_flip_stone("black",&new_board_w,&new_board_b,move,1,flip_list,&flip_list_size);
            RowCol result_act2;
            float result_score2;
            minimax2(new_board_w,new_board_b,0,alpha,beta,TRUE,true_pass,false_pass,&result_act2,&result_score2);
            move_order_list[i] = (MoveOrder){result_score2,move,new_board_w,new_board_b};
        }
        qsort(move_order_list, move_oder_list_size, sizeof(MoveOrder), compare_move_order_asc);
        for(int i=0;i<legal_size;i++){
            RowCol move = move_order_list[i].move;
            uint64_t new_board_w = move_order_list[i].newBoardW;
            uint64_t new_board_b = move_order_list[i].newBoardB;

            float eval;
            RowCol act2;

            minimax2(new_board_w,new_board_b,depth-1,alpha,beta,TRUE,true_pass,false_pass,&act2,&eval);
            //printf("\nact:(%d,%d),score:%f",move.row,move.col,eval);
            if (eval < min_eval){
                min_eval = eval;
                best_move = move;
            }
            beta = nmin(beta, eval);
            if (beta <= alpha){
                break;
            }
        }
        *act = best_move;
        *score = min_eval;
        free(move_order_list);
        free(legal_list_w);
        free(legal_list_b);
        return;
    }
}

void minimax3(uint64_t board_w,uint64_t board_b,int depth,float alpha,float beta,int maximizing_player,int true_pass,int false_pass,RowCol *act,float *score,
    float con_weight,
    float edge_weight,
    float dis_num_weight,
    float board_weight,
    float dis_sum_weight,
    float legal_dis_weight,
    float connect_weight,
    float spread_weight,
    float open_weight,
    float alpha_start,
    float alpha_speed,
    float beta_start,
    float beta_speed,
    float pass_weight,
    float touch_weight,
    float pattern_weight,
    float connect_weight2,
    float cx_point_weight,
    float corner_point_weight
){
    /* call_count++;
    printf("call #%d, depth=%d\n", call_count, depth);
    printf("board_w: 0x%016llX\n", board_w);
    printf("board_b: 0x%016llX\n", board_b); */
    //fflush(stdout);
    //printf("%d,%d\n",call_count,depth);
    /* if (call_count > 600) {
        
        printf("Recursion limit exceeded\n");
        return;
    } */
    //printf("=== minimax() 入った ===\n"); fflush(stdout);
    //printf("\nBoard Wm: %llu\n", board_w);
    //printf("order1\n");
    //printf("Board Bm: %llu\n", board_b);
    //printf("depth:%d\n",depth);
    //printf("order2\n");
    //printf("depth:%d",depth);
    //RowCol *legal_list_w = malloc(sizeof(RowCol) * 64);
    //RowCol *legal_list_b = malloc(sizeof(RowCol) * 64);
    fflush(stdout);
    RowCol legal_list_w[64];
    int legal_list_size_w = 0;
    get_legal_square("white",board_w,board_b,legal_list_w,&legal_list_size_w);
    //printf("\norder1\n");
    RowCol legal_list_b[64];
    int legal_list_size_b = 0;
    //printf("\norder2\n");
    get_legal_square("black",board_w,board_b,legal_list_b,&legal_list_size_b);
    //printf("\norder3");
    RowCol result_act = *act;
    float result_score = *score;
    if (depth == 0 || is_terminal(board_w,board_b) || (legal_list_size_w==0 && legal_list_size_b==0)){
        //printf("\n葉ノード");
        //printf("\nt:%d",true_pass);
        //printf("\nf:%d",false_pass);
        *score = evaluate_board3(board_w,board_b,true_pass,false_pass,
        con_weight,
        edge_weight,
        dis_num_weight,
        board_weight,
        dis_sum_weight,
        legal_dis_weight,
        connect_weight,
        spread_weight,
        open_weight,
        alpha_start,
        alpha_speed,
        beta_start,
        beta_speed,
        pass_weight,
        touch_weight,
        pattern_weight,
        connect_weight2,
        cx_point_weight,
        corner_point_weight);
        //fflush(stdout);
        //printf("\nscore:%f\n",*score);
        //free(legal_list_w);
        //free(legal_list_b);
        return;
    }
    
    if (maximizing_player){
        RowCol* legal_list = legal_list_w;
        int legal_size = legal_list_size_w;
        if(legal_size==0){
            minimax3(board_w,board_b,depth,alpha,beta,FALSE,true_pass+1,false_pass,&*act,&*score,
            con_weight,
            edge_weight,
            dis_num_weight,
            board_weight,
            dis_sum_weight,
            legal_dis_weight,
            connect_weight,
            spread_weight,
            open_weight,
            alpha_start,
            alpha_speed,
            beta_start,
            beta_speed,
            pass_weight,
            touch_weight,
            pattern_weight,
            connect_weight2,
            cx_point_weight,
            corner_point_weight);
            return;
        }
        float max_eval = -INFINITY;
        RowCol best_move;
        
        //RowCol move_oder_list[64];
        int move_oder_list_size = legal_size;
        MoveOrder move_order_list[64];
        //MoveOrder *move_order_list = malloc(sizeof(MoveOrder) * move_oder_list_size);
        for(int i=0;i<legal_size;i++){
            RowCol move = legal_list[i];
            RowCol flip_list[64];
            int flip_list_size;
            uint64_t new_board_w = board_w,new_board_b = board_b;
            
            fflush(stdout);
            identify_flip_stone("white",&new_board_w,&new_board_b,move,1,flip_list,&flip_list_size);
            RowCol result_act2;
            float result_score2;
            /* printf(">>> call:%d",call_count);
            printf(">>> Trying move (%d, %d) at depth=%d\n", move.row, move.col, depth);
            printf(">>> New W: 0x%016llX\n", new_board_w);
            printf(">>> New B: 0x%016llX\n", new_board_b); */
            result_score2 = evaluate_board3(board_w,board_b,true_pass,false_pass,
            con_weight,
            edge_weight,
            dis_num_weight,
            board_weight,
            dis_sum_weight,
            legal_dis_weight,
            connect_weight,
            spread_weight,
            open_weight,
            alpha_start,
            alpha_speed,
            beta_start,
            beta_speed,
            pass_weight,
            touch_weight,
            pattern_weight,
            connect_weight2,
            cx_point_weight,
            corner_point_weight);
            move_order_list[i] = (MoveOrder){result_score2,move,new_board_w,new_board_b};
        }
        qsort(move_order_list, move_oder_list_size, sizeof(MoveOrder), compare_move_order_desc);//maximizing_player＝=Falseでは昇順に
        for(int i=0;i<legal_size;i++){
            RowCol move = move_order_list[i].move;
            uint64_t new_board_w = move_order_list[i].newBoardW;
            uint64_t new_board_b = move_order_list[i].newBoardB;

            float eval;
            RowCol act2;
            if(i==0){
                minimax3(new_board_w,new_board_b,depth-1,alpha,beta,FALSE,true_pass,false_pass,&act2,&eval,
                con_weight,
                edge_weight,
                dis_num_weight,
                board_weight,
                dis_sum_weight,
                legal_dis_weight,
                connect_weight,
                spread_weight,
                open_weight,
                alpha_start,
                alpha_speed,
                beta_start,
                beta_speed,
                pass_weight,
                touch_weight,
                pattern_weight,
                connect_weight2,
                cx_point_weight,
                corner_point_weight);
            }else{
                minimax3(new_board_w,new_board_b,depth-1,alpha,alpha+1,FALSE,true_pass,false_pass,&act2,&eval,
                con_weight,
                edge_weight,
                dis_num_weight,
                board_weight,
                dis_sum_weight,
                legal_dis_weight,
                connect_weight,
                spread_weight,
                open_weight,
                alpha_start,
                alpha_speed,
                beta_start,
                beta_speed,
                pass_weight,
                touch_weight,
                pattern_weight,
                connect_weight2,
                cx_point_weight,
                corner_point_weight);
                if(eval>alpha){
                    minimax3(new_board_w,new_board_b,depth-1,alpha,beta,FALSE,true_pass,false_pass,&act2,&eval,
                    con_weight,
                    edge_weight,
                    dis_num_weight,
                    board_weight,
                    dis_sum_weight,
                    legal_dis_weight,
                    connect_weight,
                    spread_weight,
                    open_weight,
                    alpha_start,
                    alpha_speed,
                    beta_start,
                    beta_speed,
                    pass_weight,
                    touch_weight,
                    pattern_weight,
                    connect_weight2,
                    cx_point_weight,
                    corner_point_weight);
                }
            }
            /* minimax3(new_board_w,new_board_b,depth-1,alpha,beta,FALSE,true_pass,false_pass,&act2,&eval,
            con_weight,
            edge_weight,
            dis_num_weight,
            board_weight,
            dis_sum_weight,
            legal_dis_weight,
            connect_weight,
            spread_weight,
            open_weight,
            alpha_start,
            alpha_speed,
            beta_start,
            beta_speed,
            pass_weight,
            touch_weight,
            pattern_weight,
            connect_weight2,
            cx_point_weight,
            corner_point_weight); */
            if (eval > max_eval){
                max_eval = eval;
                best_move = move;
            }
            alpha = nmax(alpha, eval);
            if (beta <= alpha){
                break;
            }
        }
        //printf(">>> return action: (%d,%d)\n", best_move.row, best_move.col);
        *act = best_move;
        *score = max_eval;
        //free(move_order_list);
        //free(legal_list_w);
        //free(legal_list_b);
        return;
    }else{
        RowCol* legal_list = legal_list_b;
        int legal_size = legal_list_size_b;
        if(legal_size==0){
            minimax3(board_w,board_b,depth,alpha,beta,TRUE,true_pass,false_pass+1,&*act,&*score,
            con_weight,
            edge_weight,
            dis_num_weight,
            board_weight,
            dis_sum_weight,
            legal_dis_weight,
            connect_weight,
            spread_weight,
            open_weight,
            alpha_start,
            alpha_speed,
            beta_start,
            beta_speed,
            pass_weight,
            touch_weight,
            pattern_weight,
            connect_weight2,
            cx_point_weight,
            corner_point_weight);
            return;
        }
        float min_eval = INFINITY;
        RowCol best_move;

        int move_oder_list_size = legal_size;
        MoveOrder move_order_list[64];
        //MoveOrder *move_order_list = malloc(sizeof(MoveOrder) * move_oder_list_size);
        for(int i=0;i<legal_size;i++){
            RowCol move = legal_list[i];
            RowCol flip_list[64];
            int flip_list_size;
            uint64_t new_board_w = board_w,new_board_b = board_b;
            identify_flip_stone("black",&new_board_w,&new_board_b,move,1,flip_list,&flip_list_size);
            RowCol result_act2;
            float result_score2;
            result_score2 = evaluate_board3(board_w,board_b,true_pass,false_pass,
            con_weight,
            edge_weight,
            dis_num_weight,
            board_weight,
            dis_sum_weight,
            legal_dis_weight,
            connect_weight,
            spread_weight,
            open_weight,
            alpha_start,
            alpha_speed,
            beta_start,
            beta_speed,
            pass_weight,
            touch_weight,
            pattern_weight,
            connect_weight2,
            cx_point_weight,
            corner_point_weight);
            move_order_list[i] = (MoveOrder){result_score2,move,new_board_w,new_board_b};
        }
        qsort(move_order_list, move_oder_list_size, sizeof(MoveOrder), compare_move_order_asc);
        for(int i=0;i<legal_size;i++){
            RowCol move = move_order_list[i].move;
            uint64_t new_board_w = move_order_list[i].newBoardW;
            uint64_t new_board_b = move_order_list[i].newBoardB;

            float eval;
            RowCol act2;

            if(i==0){
                minimax3(new_board_w,new_board_b,depth-1,alpha,beta,TRUE,true_pass,false_pass,&act2,&eval,
                con_weight,
                edge_weight,
                dis_num_weight,
                board_weight,
                dis_sum_weight,
                legal_dis_weight,
                connect_weight,
                spread_weight,
                open_weight,
                alpha_start,
                alpha_speed,
                beta_start,
                beta_speed,
                pass_weight,
                touch_weight,
                pattern_weight,
                connect_weight2,
                cx_point_weight,
                corner_point_weight);
            }else{
                minimax3(new_board_w,new_board_b,depth-1,alpha,alpha+1,TRUE,true_pass,false_pass,&act2,&eval,
                con_weight,
                edge_weight,
                dis_num_weight,
                board_weight,
                dis_sum_weight,
                legal_dis_weight,
                connect_weight,
                spread_weight,
                open_weight,
                alpha_start,
                alpha_speed,
                beta_start,
                beta_speed,
                pass_weight,
                touch_weight,
                pattern_weight,
                connect_weight2,
                cx_point_weight,
                corner_point_weight);
                if(eval<beta){
                    minimax3(new_board_w,new_board_b,depth-1,alpha,beta,TRUE,true_pass,false_pass,&act2,&eval,
                    con_weight,
                    edge_weight,
                    dis_num_weight,
                    board_weight,
                    dis_sum_weight,
                    legal_dis_weight,
                    connect_weight,
                    spread_weight,
                    open_weight,
                    alpha_start,
                    alpha_speed,
                    beta_start,
                    beta_speed,
                    pass_weight,
                    touch_weight,
                    pattern_weight,
                    connect_weight2,
                    cx_point_weight,
                    corner_point_weight);
                }
            }
            /* minimax3(new_board_w,new_board_b,depth-1,alpha,beta,TRUE,true_pass,false_pass,&act2,&eval,
            con_weight,
            edge_weight,
            dis_num_weight,
            board_weight,
            dis_sum_weight,
            legal_dis_weight,
            connect_weight,
            spread_weight,
            open_weight,
            alpha_start,
            alpha_speed,
            beta_start,
            beta_speed,
            pass_weight,
            touch_weight,
            pattern_weight,
            connect_weight2,
            cx_point_weight,
            corner_point_weight); */
            //printf("\nact:(%d,%d),score:%f",move.row,move.col,eval);
            if (eval < min_eval){
                min_eval = eval;
                best_move = move;
            }
            beta = nmin(beta, eval);
            if (beta <= alpha){
                break;
            }
        }
        *act = best_move;
        *score = min_eval;
        //free(move_order_list);
        //free(legal_list_w);
        //free(legal_list_b);
        //printf(">>> return action: (%d,%d)\n", best_move.row, best_move.col);
        return;
    }
}
#define POP_SIZE 100

float rand_float(float a, float b) {
    //srand(time(NULL));
    return a + ((float)rand() / (float)RAND_MAX) * (b - a);
}
#define RAND_R_CUSTOM_MAX 0x7fffffff
unsigned int rand_r_custom(unsigned int *seed) {
    *seed = (*seed * 1103515245 + 12345) & RAND_R_CUSTOM_MAX;
    return *seed;
}

float rand_float2(float a, float b) {
    static _Thread_local unsigned int seed = 0;
    if (seed == 0) {
        seed = (unsigned int)time(NULL) ^ omp_get_thread_num();
    }
    return a + ((float)rand_r_custom(&seed) / (float)RAND_R_CUSTOM_MAX) * (b - a);
}


struct Weights {
    float con_weight;
    float edge_weight;
    float dis_num_weight;
    float board_weight;
    float dis_sum_weight;
    float legal_dis_weight;
    float connect_weight;
    float spread_weight;
    float open_weight;
    float alpha_start;
    float alpha_speed;
    float beta_start;
    float beta_speed;
    float pass_weight;
    float touch_weight;
    float pattern_weight;
    float connect_weight2;
    float cx_point_weight;
    float corner_point_weight;
};

struct Individual {
    struct Weights weights;  // 評価関数の重み
    int games;               // 総対戦数
    int wins;                // 勝利数
    float total_stone_diff;     // 総石差（自分 - 相手）
    float fitness;
};


struct Individual population[POP_SIZE];
struct Individual opponent[20];

struct Individual weights_test[9];
struct Individual population4test[POP_SIZE];

float stone_diffs[9][100000];

void test_init(){
    
    /* weights_test[0].weights.con_weight = 100;
    weights_test[0].weights.edge_weight = 0.59;
    weights_test[0].weights.dis_num_weight = 9.57;
    weights_test[0].weights.board_weight = 4.12;
    weights_test[0].weights.dis_sum_weight = -0.75;
    weights_test[0].weights.legal_dis_weight = 17.93;
    weights_test[0].weights.connect_weight = -61.89;
    weights_test[0].weights.spread_weight = 35.47;
    weights_test[0].weights.open_weight = 14.78;
    weights_test[0].weights.alpha_start = 52.14;
    weights_test[0].weights.alpha_speed = 50.78;
    weights_test[0].weights.beta_start = 6.36;
    weights_test[0].weights.beta_speed = 4.47;
    weights_test[0].weights.pass_weight = -2.24;
    weights_test[0].weights.touch_weight = -20.14;
    weights_test[0].weights.pattern_weight = -0.05;
    weights_test[0].weights.connect_weight2 = -25.13;
    weights_test[0].weights.cx_point_weight = 0.35;
    weights_test[0].weights.corner_point_weight = -5.47; */

    FILE *fp = fopen("weights1_2/weights_gen6199.bin", "rb");//64位
    fread(population4test, sizeof(struct Individual), 100, fp);
    fclose(fp);
    weights_test[0] = population4test[0];

    FILE *fp2 = fopen("weights1_2/weights_gen999.bin", "rb");//62位
    fread(population4test, sizeof(struct Individual), 100, fp2);
    fclose(fp2);
    weights_test[1] = population4test[0];

    FILE *fp3 = fopen("weights1_2/weights_gen1999.bin", "rb");
    fread(population4test, sizeof(struct Individual), 100, fp3);
    fclose(fp3);
    weights_test[2] = population4test[0];

    FILE *fp4 = fopen("weights1_2/weights_gen2999.bin", "rb");
    fread(population4test, sizeof(struct Individual), 100, fp4);
    fclose(fp4);
    weights_test[3] = population4test[0];

    FILE *fp5 = fopen("weights1_2/weights_gen3999.bin", "rb");
    fread(population4test, sizeof(struct Individual), 100, fp5);
    fclose(fp5);
    weights_test[4] = population4test[0];

    FILE *fp6 = fopen("weights1_2/weights_gen4999.bin", "rb");
    fread(population4test, sizeof(struct Individual), 100, fp6);
    fclose(fp6);
    weights_test[5] = population4test[0];

    FILE *fp7 = fopen("weights1_2/weights_gen5999.bin", "rb");
    fread(population4test, sizeof(struct Individual), 100, fp7);
    fclose(fp7);
    weights_test[6] = population4test[0];

    for(int i=0;i<8;i++){
        weights_test[i].games = 0;
        weights_test[i].wins = 0;
        weights_test[i].total_stone_diff = 0;
        weights_test[i].fitness = 0;
        weights_test[i].weights.con_weight = 100;
    }

    weights_test[7].weights.con_weight = 200;
    weights_test[7].weights.edge_weight = 0.9;
    weights_test[7].weights.dis_num_weight = 2;
    weights_test[7].weights.board_weight = 4;
    weights_test[7].weights.open_weight = 5;
    weights_test[7].weights.connect_weight = 3;
    weights_test[7].weights.spread_weight = 10;
    weights_test[7].weights.dis_sum_weight = 24/200;
    weights_test[7].weights.legal_dis_weight = 300;
    weights_test[7].weights.pass_weight = 100;
    weights_test[7].weights.alpha_start = 45;
    weights_test[7].weights.alpha_speed = 5;
    weights_test[7].weights.beta_start = 15;
    weights_test[7].weights.beta_speed = 15;
    weights_test[7].weights.touch_weight=0;
    weights_test[7].weights.pattern_weight=0;
    weights_test[7].weights.connect_weight2=0;
    weights_test[7].weights.cx_point_weight=1;
    weights_test[7].weights.corner_point_weight=0;

    weights_test[8].weights.con_weight = 100;
    weights_test[8].weights.edge_weight = 1;
    weights_test[8].weights.dis_num_weight = 8.19;
    weights_test[8].weights.board_weight = 5.93;
    weights_test[8].weights.dis_sum_weight = 10.3;
    weights_test[8].weights.legal_dis_weight = 5.33;
    weights_test[8].weights.connect_weight = 8.5;
    weights_test[8].weights.spread_weight = 10.3;
    weights_test[8].weights.open_weight = -11.71;
    weights_test[8].weights.alpha_start = 60;
    weights_test[8].weights.alpha_speed = 47.74;
    weights_test[8].weights.beta_start = 31.71;
    weights_test[8].weights.beta_speed = 10.38;
    weights_test[8].weights.pass_weight = 6.96;
    weights_test[8].weights.touch_weight = 14.53;
    weights_test[8].weights.pattern_weight = 0.04;
    weights_test[8].weights.connect_weight2 = 3.64;
    weights_test[8].weights.cx_point_weight = -0.28;
    weights_test[8].weights.corner_point_weight = 14.40; 

}
void export_diff_stone(int num, const char *filename,float diff_stone[9][100000]){
    FILE *fp = fopen(filename,"w");

    if (fp == NULL) {
        perror("Failed to open CSV file");
        return;
    }
    // ヘッダー
    fprintf(fp, "0,1,2,3,4,5,6,7,8\n");

    for (int i = 0; i < num; i++) {
        fprintf(fp, "%.2f", diff_stone[0][i]);
        for (int j = 1; j < 9; j++) {
            fprintf(fp, ",%.2f", diff_stone[j][i]);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
}
void export_topk_to_csv(const struct Individual population[], int generation, int top_k, const char *filename,struct Individual opponent[]) {
    FILE *fp = fopen(filename, generation == 0 ? "w" : "a");  // 0世代なら新規、それ以降は追記

    if (fp == NULL) {
        perror("Failed to open CSV file");
        return;
    }

    if (generation == 0) {
        // ヘッダー
        fprintf(fp, "generation,rank,fitness,wins,games,stone_diff");
        fprintf(fp, ",con_weight,edge_weight,dis_num_weight,board_weight");
        fprintf(fp, ",dis_sum_weight,legal_dis_weight,connect_weight,spread_weight");
        fprintf(fp, ",open_weight,alpha_start,alpha_speed,beta_start,beta_speed,pass_weight");
        fprintf(fp, ",touch_weight,pattern_weight,connect_weight2,cx_point_weight,corner_point_weight\n");
    }

    for (int i = 0; i < top_k; i++) {
        const struct Weights *w = &population[i].weights;
        fprintf(fp, "%d,%d,%.2f,%d,%d,%.2f", generation, i + 1, population[i].fitness,
                population[i].wins, population[i].games, population[i].total_stone_diff);

        fprintf(fp, ",%.2f,%.2f,%.2f,%.2f", w->con_weight, w->edge_weight, w->dis_num_weight, w->board_weight);
        fprintf(fp, ",%.2f,%.2f,%.2f,%.2f", w->dis_sum_weight, w->legal_dis_weight, w->connect_weight, w->spread_weight);
        fprintf(fp, ",%.2f,%.2f,%.2f,%.2f,%.2f,%.2f", w->open_weight, w->alpha_start, w->alpha_speed, w->beta_start, w->beta_speed, w->pass_weight);
        fprintf(fp, ",%.2f,%.2f,%.2f,%.2f,%.2f\n", w->touch_weight, w->pattern_weight, w->connect_weight2, w->cx_point_weight, w->corner_point_weight);
    }

    fclose(fp);

    if(generation%100==99){
        char bin_filename[256];
        snprintf(bin_filename, sizeof(bin_filename), "weights1_2/weights_gen%03d.bin", generation);

        FILE *bin_fp = fopen(bin_filename, "wb");
        if (!bin_fp) {
            perror("Failed to open binary file");
            return;
        }

        fwrite(population, sizeof(struct Individual), POP_SIZE, bin_fp);
        fclose(bin_fp);
    }
    if(generation%100==99){
        char bin_filename2[256];
        snprintf(bin_filename2, sizeof(bin_filename2), "opponents1_2/opponent_gen%03d.bin", generation);

        FILE *bin_fp = fopen(bin_filename2, "wb");
        if (!bin_fp) {
            perror("Failed to open binary file");
            return;
        }

        fwrite(opponent, sizeof(struct Individual), 20, bin_fp);
        fclose(bin_fp);
    }
}
void compute_fitness(struct Individual population[], int size) {
    for (int i = 0; i < size; i++) {
        if (population[i].games > 0) {
            float win_rate = (float)population[i].wins / population[i].games;
            population[i].fitness = win_rate * win_rate * ((float)population[i].total_stone_diff);
            if(win_rate==0){
                population[i].fitness = (float)population[i].total_stone_diff;
            }
        } else {
            population[i].fitness = -9999;  // 未対戦個体は最低評価
        }
    }
}
void progress_game_random(struct Individual individual1,int random_color,int *winner,float *diff_stone){//1が先手(黒)
    uint64_t current_w = first_board_w;
    uint64_t current_b = first_board_b;
    int current_color = -1;
    int turn = 0;
    while (TRUE){
        RowCol legal_list[64];
        int legal_list_size;
        if(current_color == -1){
            get_legal_square("black",current_w,current_b,legal_list,&legal_list_size);
        }else{
            get_legal_square("white",current_w,current_b,legal_list,&legal_list_size);
        }

        if(legal_list_size==0){
            current_color *= -1;
            RowCol legal_list[64];
            int legal_list_size;
            if(current_color == -1){
                get_legal_square("black",current_w,current_b,legal_list,&legal_list_size);
            }else{
                get_legal_square("white",current_w,current_b,legal_list,&legal_list_size);
            }
            if(legal_list_size==0){
                break;
            }
            continue;
        }
        
        RowCol act;
        float score;
        if(current_color==-1&&random_color==1){
            minimax3(current_b,current_w,2,-INFINITY,INFINITY,TRUE,0,0,&act,&score,
            individual1.weights.con_weight,
            individual1.weights.edge_weight,
            individual1.weights.dis_num_weight,
            individual1.weights.board_weight,
            individual1.weights.dis_sum_weight,
            individual1.weights.legal_dis_weight,
            individual1.weights.connect_weight,
            individual1.weights.spread_weight,
            individual1.weights.open_weight,
            individual1.weights.alpha_start,
            individual1.weights.alpha_speed,
            individual1.weights.beta_start,
            individual1.weights.beta_speed,
            individual1.weights.pass_weight,
            individual1.weights.touch_weight,
            individual1.weights.pattern_weight,
            individual1.weights.connect_weight2,
            individual1.weights.cx_point_weight,
            individual1.weights.corner_point_weight);
        }else if(current_color==1&&random_color==-1){
            minimax3(current_w,current_b,2,-INFINITY,INFINITY,TRUE,0,0,&act,&score,
            individual1.weights.con_weight,
            individual1.weights.edge_weight,
            individual1.weights.dis_num_weight,
            individual1.weights.board_weight,
            individual1.weights.dis_sum_weight,
            individual1.weights.legal_dis_weight,
            individual1.weights.connect_weight,
            individual1.weights.spread_weight,
            individual1.weights.open_weight,
            individual1.weights.alpha_start,
            individual1.weights.alpha_speed,
            individual1.weights.beta_start,
            individual1.weights.beta_speed,
            individual1.weights.pass_weight,
            individual1.weights.touch_weight,
            individual1.weights.pattern_weight,
            individual1.weights.connect_weight2,
            individual1.weights.cx_point_weight,
            individual1.weights.corner_point_weight);
        }else{
            act = legal_list[(int)rand_float2(0, legal_list_size)];
        }
        RowCol flip_list[64];
        int flip_list_size;
        if(current_color==-1){
            identify_flip_stone("black",&current_w,&current_b,act,0,flip_list,&flip_list_size);
        }else{
            identify_flip_stone("white",&current_w,&current_b,act,0,flip_list,&flip_list_size);
        }
        turn++;
        current_color *=-1;

    }
    int num_b = bit_count(current_b);
    int num_w = bit_count(current_w);
    if(num_b>num_w){
        *winner = 1;//(先手の勝利)
        *diff_stone = (num_b - num_w) * (60.0f / (turn + 1));
    }else if(num_w>num_b){
        *winner = 2;//(後手の勝利)
        *diff_stone = (num_w - num_b) * (60.0f / (turn + 1));
    }else{
        *winner = 0;
        *diff_stone = 0;
    }

}
void test_main(){
    test_init();
    for(int i=0;i<9;i++){
        printf("\n%d",i);
        #pragma omp parallel for
        for(int k=0;k<50000;k++){
            int winner = 0;
            float diff_stone = 0;
            progress_game_random(weights_test[i],1,&winner,&diff_stone);
            weights_test[i].games++;
            
            if(winner==1){
                weights_test[i].wins += 1;

                weights_test[i].total_stone_diff += diff_stone;
                stone_diffs[i][k*2] = diff_stone;
            }else if(winner==2){
                weights_test[i].total_stone_diff -= diff_stone;
                stone_diffs[i][k*2] = -diff_stone;
            }
            progress_game_random(weights_test[i],-1,&winner,&diff_stone);
            weights_test[i].games++;
            
            if(winner==2){
                weights_test[i].wins += 1;

                weights_test[i].total_stone_diff += diff_stone;
                stone_diffs[i][k*2+1] = diff_stone;
            }else if(winner==1){
                weights_test[i].total_stone_diff -= diff_stone;
                stone_diffs[i][k*2+1] = -diff_stone;
            }
        }
    }
    compute_fitness(weights_test, 9);
    export_topk_to_csv(weights_test,0,9,"csvs/test_result.csv",weights_test);
    export_diff_stone(100000,"csvs/diff_stone.csv",stone_diffs);
}

void GA_init(){
    /* srand(time(NULL));
    FILE *fp = fopen("opponents2/opponent_gen1699.bin", "rb");
    fread(opponent, sizeof(struct Individual), 20, fp);
    fclose(fp); */
    /* opponent[0].weights.con_weight=14.36;
    opponent[0].weights.edge_weight=0.25;
    opponent[0].weights.dis_num_weight=18.78;
    opponent[0].weights.board_weight=4.67;
    opponent[0].weights.dis_sum_weight=4.02;
    opponent[0].weights.legal_dis_weight=14.48;
    opponent[0].weights.connect_weight=4.06;
    opponent[0].weights.spread_weight=7.82;
    opponent[0].weights.open_weight=15.06;
    opponent[0].weights.alpha_start=34.74;
    opponent[0].weights.alpha_speed=18.92;
    opponent[0].weights.beta_start=11.10;
    opponent[0].weights.beta_speed=28.19;
    opponent[0].weights.pass_weight=14.45;
    opponent[0].weights.touch_weight=0;
    opponent[0].weights.pattern_weight=0;
    opponent[0].weights.connect_weight2=0;
    opponent[0].weights.cx_point_weight=1;
    opponent[0].weights.corner_point_weight=0; */

    opponent[18].weights.con_weight = 200;
    opponent[18].weights.edge_weight = 0.9;
    opponent[18].weights.dis_num_weight = 2;
    opponent[18].weights.board_weight = 4;
    opponent[18].weights.open_weight = 5;
    opponent[18].weights.connect_weight = 3;
    opponent[18].weights.spread_weight = 10;
    opponent[18].weights.dis_sum_weight = 24;
    opponent[18].weights.legal_dis_weight = 300;
    opponent[18].weights.pass_weight = 100;
    opponent[18].weights.alpha_start = 45;
    opponent[18].weights.alpha_speed = 5;
    opponent[18].weights.beta_start = 15;
    opponent[18].weights.beta_speed = 15;
    opponent[18].weights.touch_weight=0;
    opponent[18].weights.pattern_weight=0;
    opponent[18].weights.connect_weight2=0;
    opponent[18].weights.cx_point_weight=1;
    opponent[18].weights.corner_point_weight=0;

    opponent[19].weights.con_weight = 100;
    opponent[19].weights.edge_weight = 1;
    opponent[19].weights.dis_num_weight = 8.19;
    opponent[19].weights.board_weight = 5.93;
    opponent[19].weights.dis_sum_weight = 10.3;
    opponent[19].weights.legal_dis_weight = 5.33;
    opponent[19].weights.connect_weight = 8.5;
    opponent[19].weights.spread_weight = 10.3;
    opponent[19].weights.open_weight = -11.71;
    opponent[19].weights.alpha_start = 60;
    opponent[19].weights.alpha_speed = 47.74;
    opponent[19].weights.beta_start = 31.71;
    opponent[19].weights.beta_speed = 10.38;
    opponent[19].weights.pass_weight = 6.96;
    opponent[19].weights.touch_weight = 14.53;
    opponent[19].weights.pattern_weight = 0.04;
    opponent[19].weights.connect_weight2 = 3.64;
    opponent[19].weights.cx_point_weight = -0.28;
    opponent[19].weights.corner_point_weight = 14.40;

    for(int i=0;i<18;i++){
        opponent[i].weights.con_weight = 100;
        opponent[i].weights.edge_weight = rand_float(-10, 10);
        opponent[i].weights.dis_num_weight = rand_float(-10, 10);
        opponent[i].weights.board_weight = rand_float(-10, 10);
        opponent[i].weights.dis_sum_weight = rand_float(-10, 10);
        opponent[i].weights.legal_dis_weight = rand_float(-10, 10);
        opponent[i].weights.connect_weight = rand_float(-10, 10);
        opponent[i].weights.spread_weight = rand_float(-10, 10);
        opponent[i].weights.open_weight = rand_float(-10, 10);
        opponent[i].weights.alpha_start = rand_float(1, 60);
        opponent[i].weights.alpha_speed = rand_float(1, 60);
        opponent[i].weights.beta_start = rand_float(1, 60);
        opponent[i].weights.beta_speed = rand_float(1, 60);
        opponent[i].weights.pass_weight = rand_float(-10, 10);
        opponent[i].weights.touch_weight=rand_float(-10, 10);
        opponent[i].weights.pattern_weight=rand_float(-10, 10);
        opponent[i].weights.connect_weight2=rand_float(-10, 10);
        opponent[i].weights.cx_point_weight=rand_float(-10, 10);
        opponent[i].weights.corner_point_weight=rand_float(-10, 10);
        //printf("%2.f",opponent[i].weights.edge_weight);
    }
    

    /* opponent[0].weights.con_weight = 33;
    opponent[0].weights.edge_weight = 1.18;
    opponent[0].weights.dis_num_weight = 8.22;
    opponent[0].weights.board_weight = 12.73;
    opponent[0].weights.open_weight = 7.13;
    opponent[0].weights.connect_weight = 7.96;
    opponent[0].weights.spread_weight = 4.91;
    opponent[0].weights.dis_sum_weight = 8.70;
    opponent[0].weights.legal_dis_weight = 4.77;
    opponent[0].weights.pass_weight = 34.62;
    opponent[0].weights.alpha_start = 46.29;
    opponent[0].weights.alpha_speed = 25.55;
    opponent[0].weights.beta_start = 29.01;
    opponent[0].weights.beta_speed = 2.22;
    opponent[0].weights.touch_weight=2.44;
    opponent[0].weights.pattern_weight=0.30;
    opponent[0].weights.connect_weight2=3.46;
    opponent[0].weights.cx_point_weight=2.47;
    opponent[0].weights.corner_point_weight=3.43; */

    
    /* FILE *fp2 = fopen("weights2/weights_gen1699.bin", "rb");
    fread(population, sizeof(struct Individual), 100, fp2);
    fclose(fp2); */

    
    for(int i=0;i<POP_SIZE;i++){
        population[i].games = 0;
        population[i].wins = 0;
        population[i].total_stone_diff = 0;
        population[i].fitness = 0;
        population[i].weights.con_weight = 100;
        population[i].weights.edge_weight = rand_float(-10, 10);
        population[i].weights.dis_num_weight = rand_float(-10, 10);
        population[i].weights.board_weight = rand_float(-10, 10);
        population[i].weights.dis_sum_weight = rand_float(-10, 10);
        population[i].weights.legal_dis_weight = rand_float(-10, 10);
        population[i].weights.connect_weight = rand_float(-10, 10);
        population[i].weights.spread_weight = rand_float(-10, 10);
        population[i].weights.open_weight = rand_float(-10, 10);
        population[i].weights.alpha_start = rand_float(1, 60);
        population[i].weights.alpha_speed = rand_float(1, 60);
        population[i].weights.beta_start = rand_float(1, 60);
        population[i].weights.beta_speed = rand_float(1, 60);
        population[i].weights.pass_weight = rand_float(-10, 10);
        population[i].weights.touch_weight=rand_float(-10, 10);
        population[i].weights.pattern_weight=rand_float(-10, 10);
        population[i].weights.connect_weight2=rand_float(-10, 10);
        population[i].weights.cx_point_weight=rand_float(-10, 10);
        population[i].weights.corner_point_weight=rand_float(-10, 10);
        /* if(0<i){
            population[i].weights.con_weight = 100;
            population[i].weights.edge_weight = rand_float(-10, 100);
            population[i].weights.dis_num_weight = rand_float(-10, 100);
            population[i].weights.board_weight = rand_float(-10, 100);
            population[i].weights.dis_sum_weight = rand_float(-10, 100);
            population[i].weights.legal_dis_weight = rand_float(-10, 100);
            population[i].weights.connect_weight = rand_float(-10, 100);
            population[i].weights.spread_weight = rand_float(-10, 100);
            population[i].weights.open_weight = rand_float(-10, 100);
            population[i].weights.alpha_start = rand_float(1, 60);
            population[i].weights.alpha_speed = rand_float(1, 60);
            population[i].weights.beta_start = rand_float(1, 60);
            population[i].weights.beta_speed = rand_float(1, 60);
            population[i].weights.pass_weight = rand_float(-10, 100);
            population[i].weights.touch_weight=rand_float(-10, 100);
            population[i].weights.pattern_weight=rand_float(-10, 10);
            population[i].weights.connect_weight2=rand_float(-10, 10);
            population[i].weights.cx_point_weight=rand_float(-10, 10);
            population[i].weights.corner_point_weight=rand_float(-10, 100);
        } */
        /* if(i==0){
            population[i].games = 0;
            population[i].wins = 0;
            population[i].total_stone_diff = 0;
            population[i].fitness = 0;
            population[i].weights.con_weight=14.36;
            population[i].weights.edge_weight=0.25;
            population[i].weights.dis_num_weight=18.78;
            population[i].weights.board_weight=4.67;
            population[i].weights.dis_sum_weight=4.02;
            population[i].weights.legal_dis_weight=14.48;
            population[i].weights.connect_weight=4.06;
            population[i].weights.spread_weight=7.82;
            population[i].weights.open_weight=15.06;
            population[i].weights.alpha_start=34.74;
            population[i].weights.alpha_speed=18.92;
            population[i].weights.beta_start=11.10;
            population[i].weights.beta_speed=28.19;
            population[i].weights.pass_weight=14.45;
            population[i].weights.touch_weight=0;
            population[i].weights.pattern_weight=0;
            population[i].weights.connect_weight2=0;
            population[i].weights.cx_point_weight=1;
            population[i].weights.corner_point_weight=0;
        }else if(i==1){
            population[i].weights.con_weight = 33;
            population[i].weights.edge_weight = 1.18;
            population[i].weights.dis_num_weight = 8.22;
            population[i].weights.board_weight = 12.73;
            population[i].weights.open_weight = 7.13;
            population[i].weights.connect_weight = 7.96;
            population[i].weights.spread_weight = 4.91;
            population[i].weights.dis_sum_weight = 8.70;
            population[i].weights.legal_dis_weight = 4.77;
            population[i].weights.pass_weight = 34.62;
            population[i].weights.alpha_start = 46.29;
            population[i].weights.alpha_speed = 25.55;
            population[i].weights.beta_start = 29.01;
            population[i].weights.beta_speed = 2.22;
            population[i].weights.touch_weight=2.44;
            population[i].weights.pattern_weight=0.30;
            population[i].weights.connect_weight2=3.46;
            population[i].weights.cx_point_weight=2.47;
            population[i].weights.corner_point_weight=3.43;
        }else{
            population[i].games = 0;
            population[i].wins = 0;
            population[i].total_stone_diff = 0;
            population[i].fitness = 0;
            population[i].weights.con_weight = 100;
            population[i].weights.edge_weight = rand_float(-10, 10);
            population[i].weights.dis_num_weight = rand_float(-10, 10);
            population[i].weights.board_weight = rand_float(-10, 10);
            population[i].weights.dis_sum_weight = rand_float(-10, 10);
            population[i].weights.legal_dis_weight = rand_float(-10, 10);
            population[i].weights.connect_weight = rand_float(-10, 10);
            population[i].weights.spread_weight = rand_float(-10, 10);
            population[i].weights.open_weight = rand_float(-10, 10);
            population[i].weights.alpha_start = rand_float(1, 60);
            population[i].weights.alpha_speed = rand_float(1, 60);
            population[i].weights.beta_start = rand_float(1, 60);
            population[i].weights.beta_speed = rand_float(1, 60);
            population[i].weights.pass_weight = rand_float(-10, 10);
            population[i].weights.touch_weight=rand_float(-10, 10);
            population[i].weights.pattern_weight=rand_float(-10, 10);
            population[i].weights.connect_weight2=rand_float(-10, 10);
            population[i].weights.cx_point_weight=rand_float(-10, 10);
            population[i].weights.corner_point_weight=rand_float(-10, 10);
        } */
        
    }
    population[99].weights.con_weight = 100;
    population[99].weights.edge_weight = 1;
    population[99].weights.dis_num_weight = 8.19;
    population[99].weights.board_weight = 5.93;
    population[99].weights.dis_sum_weight = 10.3;
    population[99].weights.legal_dis_weight = 5.33;
    population[99].weights.connect_weight = 8.5;
    population[99].weights.spread_weight = 10.3;
    population[99].weights.open_weight = -11.71;
    population[99].weights.alpha_start = 60;
    population[99].weights.alpha_speed = 47.74;
    population[99].weights.beta_start = 31.71;
    population[99].weights.beta_speed = 10.38;
    population[99].weights.pass_weight = 6.96;
    population[99].weights.touch_weight = 14.53;
    population[99].weights.pattern_weight = 0.04;
    population[99].weights.connect_weight2 = 3.64;
    population[99].weights.cx_point_weight = -0.28;
    population[99].weights.corner_point_weight = 14.40;
}


void progress_game(struct Individual individual1,struct Individual individual2,int *winner,float *diff_stone){//1が先手(黒)
    printf("\n,%.2f,%.2f,%.2f,%.2f", individual1.weights.con_weight, individual1.weights.edge_weight, individual1.weights.dis_num_weight, individual1.weights.board_weight);
    printf(",%.2f,%.2f,%.2f,%.2f", individual1.weights.dis_sum_weight, individual1.weights.legal_dis_weight, individual1.weights.connect_weight, individual1.weights.spread_weight);
    printf(",%.2f,%.2f,%.2f,%.2f,%.2f,%.2f", individual1.weights.open_weight, individual1.weights.alpha_start, individual1.weights.alpha_speed, individual1.weights.beta_start, individual1.weights.beta_speed, individual1.weights.pass_weight);
    printf(",%.2f,%.2f,%.2f,%.2f,%.2f\n", individual1.weights.touch_weight, individual1.weights.pattern_weight, individual1.weights.connect_weight2, individual1.weights.cx_point_weight, individual1.weights.corner_point_weight);
    uint64_t current_w = first_board_w;
    uint64_t current_b = first_board_b;
    int current_color = -1;
    int turn = 0;
    //printf("start\n");
    while (TRUE){
        RowCol legal_list[64];
        int legal_list_size;
        if(current_color == -1){
            get_legal_square("black",current_w,current_b,legal_list,&legal_list_size);
        }else{
            get_legal_square("white",current_w,current_b,legal_list,&legal_list_size);
        }

        if(legal_list_size==0){
            current_color *= -1;
            RowCol legal_list[64];
            int legal_list_size;
            if(current_color == -1){
                get_legal_square("black",current_w,current_b,legal_list,&legal_list_size);
            }else{
                get_legal_square("white",current_w,current_b,legal_list,&legal_list_size);
            }
            if(legal_list_size==0){
                break;
            }
            continue;
        }
        RowCol act;
        float score;
        if(current_color==-1){
            minimax3(current_b,current_w,2,-INFINITY,INFINITY,TRUE,0,0,&act,&score,
            individual1.weights.con_weight,
            individual1.weights.edge_weight,
            individual1.weights.dis_num_weight,
            individual1.weights.board_weight,
            individual1.weights.dis_sum_weight,
            individual1.weights.legal_dis_weight,
            individual1.weights.connect_weight,
            individual1.weights.spread_weight,
            individual1.weights.open_weight,
            individual1.weights.alpha_start,
            individual1.weights.alpha_speed,
            individual1.weights.beta_start,
            individual1.weights.beta_speed,
            individual1.weights.pass_weight,
            individual1.weights.touch_weight,
            individual1.weights.pattern_weight,
            individual1.weights.connect_weight2,
            individual1.weights.cx_point_weight,
            individual1.weights.corner_point_weight);
        }else{
            minimax3(current_w,current_b,2,-INFINITY,INFINITY,TRUE,0,0,&act,&score,
            individual2.weights.con_weight,
            individual2.weights.edge_weight,
            individual2.weights.dis_num_weight,
            individual2.weights.board_weight,
            individual2.weights.dis_sum_weight,
            individual2.weights.legal_dis_weight,
            individual2.weights.connect_weight,
            individual2.weights.spread_weight,
            individual2.weights.open_weight,
            individual2.weights.alpha_start,
            individual2.weights.alpha_speed,
            individual2.weights.beta_start,
            individual2.weights.beta_speed,
            individual2.weights.pass_weight,
            individual2.weights.touch_weight,
            individual2.weights.pattern_weight,
            individual2.weights.connect_weight2,
            individual2.weights.cx_point_weight,
            individual2.weights.corner_point_weight);
        }
        RowCol flip_list[64];
        int flip_list_size;
        if(current_color==-1){
            identify_flip_stone("black",&current_w,&current_b,act,0,flip_list,&flip_list_size);
        }else{
            identify_flip_stone("white",&current_w,&current_b,act,0,flip_list,&flip_list_size);
        }
        turn ++;
        current_color *=-1;

    }
    int num_b = bit_count(current_b);
    int num_w = bit_count(current_w);
    if(num_b>num_w){
        *winner = 1;//(先手の勝利)
        *diff_stone = (num_b - num_w) * (60.0f / (turn + 1));
    }else if(num_w>num_b){
        *winner = 2;//(後手の勝利)
        *diff_stone = (num_w - num_b) * (60.0f / (turn + 1));
    }else{
        *winner = 0;
        *diff_stone = 0;
    }

}



int compare_individuals(const void *a, const void *b) {
    const struct Individual *indA = (const struct Individual *)a;
    const struct Individual *indB = (const struct Individual *)b;
    // 大きい順に並び替える
    if (indB->fitness > indA->fitness) return 1;
    if (indB->fitness < indA->fitness) return -1;
    return 0;
}



void compute_fitness_and_sort(struct Individual population[], int size) {
    compute_fitness(population,size);

    qsort(population, size, sizeof(struct Individual), compare_individuals);
}




struct Weights crossover(struct Weights parent1, struct Weights parent2) {
    struct Weights child;
    float *p1 = (float*)&parent1;
    float *p2 = (float*)&parent2;
    float *c  = (float*)&child;
    for (int i = 0; i < 19; i++) {
        float ratio = rand() / (float)RAND_MAX;  // 0.0～1.0
        c[i] = p1[i] * ratio + p2[i] * (1.0f - ratio);
    }
    return child;
}

void mutate(struct Weights *w, float mutation_rate, float mutation_strength) {
    float *vals = (float*)w;
    for (int i = 0; i < 19; i++) {
        if ((rand() / (float)RAND_MAX) < mutation_rate) {
            float delta = ((rand() / (float)RAND_MAX) * 2 - 1) * mutation_strength;
            
            

            // 範囲に収める（例：1〜1000 or 1〜64）
            if ((i < 9 || i>=13)) {  // 重み系
                vals[i] += delta;
            }else {  // alpha/betaパラメータ
                vals[i] += delta*5;
                if (vals[i] < 1) vals[i] = 1;
                if (vals[i] > 60) vals[i] = 60;
            }
        }
    }
}

#define TOP_K 5
//#define POP_SIZE 100
#define MUTATION_RATE 0.2f
#define MUTATION_STRENGTH 5.0f
#define NEW_RANDOM_INDIVIDUALS 30

struct Individual tournament_selection(struct Individual pop[], int k) {
    struct Individual best;
    best.fitness = -INFINITY;
    for (int i = 0; i < k; i++) {
        int idx = rand() % POP_SIZE;
        if (pop[idx].fitness > best.fitness) {
            best = pop[idx];
        }
    }
    return best;
}

void generate_next_generation(struct Individual population[]) {
    struct Individual new_population[POP_SIZE];

    // 上位TOP_Kをそのままコピー（エリート保存）
    for (int i = 0; i < TOP_K; i++) {
        new_population[i] = population[i];
        new_population[i].wins = 0;
        new_population[i].games = 0;
        new_population[i].total_stone_diff = 0;
    }
    for (int i = TOP_K; i < TOP_K*2; i++) {
        new_population[i] = population[i-TOP_K];
        new_population[i].wins = 0;
        new_population[i].games = 0;
        new_population[i].total_stone_diff = 0;
        mutate(&new_population[i].weights, MUTATION_RATE*1.5, MUTATION_STRENGTH*2);
    }
    for (int i = TOP_K*2; i < TOP_K*2 + NEW_RANDOM_INDIVIDUALS; i++) {
        new_population[i].games = 0;
        new_population[i].wins = 0;
        new_population[i].total_stone_diff = 0;
        new_population[i].fitness = 0;
        new_population[i].weights.con_weight = 100;
        new_population[i].weights.edge_weight = rand_float(-10, 10);
        new_population[i].weights.dis_num_weight = rand_float(-10, 10);
        new_population[i].weights.board_weight = rand_float(-10, 10);
        new_population[i].weights.dis_sum_weight = rand_float(-10, 10);
        new_population[i].weights.legal_dis_weight = rand_float(-10, 10);
        new_population[i].weights.connect_weight = rand_float(-10, 10);
        new_population[i].weights.spread_weight = rand_float(-10, 10);
        new_population[i].weights.open_weight = rand_float(-10, 10);
        new_population[i].weights.alpha_start = rand_float(1, 60);
        new_population[i].weights.alpha_speed = rand_float(1, 60);
        new_population[i].weights.beta_start = rand_float(1, 60);
        new_population[i].weights.beta_speed = rand_float(1, 60);
        new_population[i].weights.pass_weight = rand_float(-10, 10);
        new_population[i].weights.touch_weight=rand_float(-10, 10);
        new_population[i].weights.pattern_weight=rand_float(-10, 10);
        new_population[i].weights.connect_weight2=rand_float(-10, 10);
        new_population[i].weights.cx_point_weight=rand_float(-10, 10);
        new_population[i].weights.corner_point_weight=rand_float(-10, 10);
    }
    // 残りは交叉＋突然変異で生成
    for (int i = TOP_K*2+NEW_RANDOM_INDIVIDUALS; i < POP_SIZE; i++) {
        /* int parent1_idx = rand() % TOP_K;
        int parent2_idx = rand() % TOP_K; */
        /* struct Weights child_weights = crossover(population[parent1_idx].weights,
                                                population[parent2_idx].weights); */
        struct Weights child_weights = crossover(tournament_selection(population,3).weights,
                                                tournament_selection(population,3).weights);
        mutate(&child_weights, MUTATION_RATE, MUTATION_STRENGTH);
        new_population[i].weights.con_weight = 100;
        new_population[i].weights = child_weights;
        new_population[i].wins = 0;
        new_population[i].games = 0;
        new_population[i].total_stone_diff = 0;
    }

    // 新しい個体群を反映
    memcpy(population, new_population, sizeof(new_population));
}






void GA_main() {
    GA_init();
    
    for(int gen=0;gen<20000;gen++){
        int winner = 0;
        float diff_stone = 0;
        #pragma omp parallel for
        for(int i=0;i<POP_SIZE;i++){
            printf("%dvsopp\n",i);
            for(int opp=0;opp<20;opp++){
            /* for(int kk=0;kk<(POP_SIZE-i);kk++){
                int k = (kk+i);
                printf("%dvs%d\n",i,k); */
                
                winner = 0;    
                diff_stone = 0;
                progress_game(population[i],opponent[opp],&winner,&diff_stone);
                //printf("progress_game");
                population[i].games += 1;
                if(winner==1){
                    population[i].wins += 1;

                    population[i].total_stone_diff += diff_stone;
                }else if(winner==2){
                    population[i].total_stone_diff -= diff_stone;
                }

                winner = 0;    
                diff_stone = 0;
                progress_game(opponent[opp],population[i],&winner,&diff_stone);
                //printf("progress_game");
                population[i].games += 1;
                if(winner==2){
                    population[i].wins += 1;

                    population[i].total_stone_diff += diff_stone;
                }else if(winner==1){
                    population[i].total_stone_diff -= diff_stone;
                }
                
            }
            for(int k=0;k<10;k++){
                    winner = 0;    
                    diff_stone = 0;
                    progress_game_random(population[i],1,&winner,&diff_stone);//1=individual
                    //printf("progress_game_random");
                    population[i].games += 1;
                    if(winner==1){
                        population[i].wins += 1;

                        population[i].total_stone_diff += diff_stone;
                    }else if(winner==2){
                        population[i].total_stone_diff -= diff_stone;
                    }
                    winner = 0;    
                    diff_stone = 0;
                    progress_game_random(population[i],-1,&winner,&diff_stone);//2=individual
                    //printf("progress_game_random");

                    population[i].games += 1;
                    if(winner==2){
                        population[i].wins += 1;

                        population[i].total_stone_diff += diff_stone;
                    }else if(winner==1){
                        population[i].total_stone_diff -= diff_stone;
                    }
                }
            //}
        }
        //progress_game_random(population[0],1,&winner,&diff_stone);
        //printf("winner:%d,diff:%d\n",winner,diff_stone);

        compute_fitness_and_sort(population, POP_SIZE);
        printf("Best fitness: %.2f\n", population[0].fitness);
        if(gen % 100==99){
            for(int i=0;i<19;i++){
                opponent[i] = opponent[i+1];
            }
            opponent[19] = population[0];
        }
        export_topk_to_csv(population, gen, TOP_K, "csvs/top_individuals.csv",opponent);
        generate_next_generation(population);
        
    }
    /* for (int i = 0; i < POP_SIZE; i++) {
        printf("Individual %d: games=%d,wins=%d: total_stone_diff=%d: fitness=%2.f\n",
                i, population[i].games,population[i].wins, population[i].total_stone_diff,population[i].fitness);
    } */
}


int main(void){
    //omp_set_num_threads(12);
    //printf("LEFT_MASK = %llu\n", LEFT_MASK&RIGHT_MASK);
    RowCol legal_list[64];
    int legal_list_size;
    //get_legal_square("black",68853694464,34628173824,legal_list,&legal_list_size);
    white = 68853694464ULL;
    black = 34628173824ULL;
    RowCol flip_list[64];
    int flip_list_size;


    
    //identify_flip_stone("black",&white,&black,"d3",1,legal_list,&flip_list_size);
    //printf("\n%llu\n%llu",white,black);
    //printf(legal_list);
    /* printf("\n");
    for (int i = 0; i < legal_list_size; i++) {
        printf("Legal Move: Row = %d, Col = %d\n", legal_list[i].row, legal_list[i].col);
    } */
    /* for (int i = 0; i < flip_list_size; i++) {
        printf("\nFlip Stone: Row = %d, Col = %d\n", legal_list[i].row, legal_list[i].col);
    } */
    white = 68761356292ULL;
    black = 34829500416ULL;
    white = 8952118241016364412ULL;
    black = 193113006800896ULL;
    white = 8952118241016364415ULL;
    black = 193113006800896ULL;
    printf("%2.f",evaluate_board3(white,black,0,0,100,1,8.19,5.93,10.3,5.33,8.5,10.3,-11.7,60,47.74,31.71,10.38,6.96,14.53,0.04,3.64,-0.28,14.4));
    printf("\n%f",evaluate_board(white,black,0,0));
    uint64_t c_w=0,c_b=0;
    get_confirmed_stones(white,black,&c_w,&c_b);
    printf("\n%llu,%llu",c_w,c_b);

    RowCol act;
    float score;
    minimax(white,black,3,-INFINITY,INFINITY,TRUE,0,0,&act,&score);
    printf("\nfinal act:(%d,%d),score:%f",act.row,act.col,score);
    printf("\nscore:%f",evaluate_board5(white,black,0,0));
    printf("\nscore:%f",position_point(white));
    
    printf("\nw:%f",calc_spread_penalty(white));
    printf("\nb:%f",calc_spread_penalty(black));
    //GA_main();
    //GA_init();
    //test_main();
    return 0;
}