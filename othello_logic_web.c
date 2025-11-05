#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
/* #include <time.h>
#include <omp.h>  */

#define first_board_w 68853694464ULL
#define first_board_b 34628173824ULL

uint64_t white,black;
#define LEFT_MASK 0x7F7F7F7F7F7F7F7F// 左端を0にする
#define RIGHT_MASK 0xFEFEFEFEFEFEFEFE// 右端を0にする
#define safety_mask 0xFFFFFFFFFFFFFFFF

#define FALSE   0
#define TRUE    1
#define BOARD_SIZE 64
#define EMPTY 0
#define BLACK 1
#define WHITE 2
#define FLAG_EXACT 0
#define FLAG_LOWERBOUND 1
#define FLAG_UPPERBOUND 2
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
    13635773771771019264ULL ,
    9223583970954838144ULL ,
    15549ULL ,
    72060905508241409ULL ,
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

float position_point(uint64_t board){
    float score = 0;
    for(int i=0;i<56;i++){
        if((board&masks[i])==masks[i]){
            score += counts[i];
        }
    }
    return score;
}

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

void find_connected_open_stone(uint64_t board_w,uint64_t board_b, uint64_t empty, float *count, float *count_open, float *touch_other) {
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

float evaluate_board_old(uint64_t board_w,uint64_t board_b,float true_pass,float false_pass){
    int con_weight = 100;
    uint64_t empty =  ~(board_w | board_b)& 0xFFFFFFFFFFFFFFFF;
    int turn = 64 - bit_count(empty);
    float alpha = nmin(nmax(0,(turn -35) / 20),1);

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

    int b_snum = bit_count(board_b);
    int w_snum = bit_count(board_w);
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
float evaluate_board(uint64_t board_w,uint64_t board_b,float true_pass,float false_pass){

    float con_weight = 100;
    float edge_weight = 0.47;
    float dis_num_weight = 13.62;
    float board_weight = 3.77;
    float dis_sum_weight = -3.12;
    float legal_dis_weight = -6.76;
    float connect_weight = -54;
    float spread_weight = -23.14;
    float open_weight = -33.01;
    float alpha_start = 59.9;
    float alpha_speed = 46.93;
    float beta_start = 1.77;
    float beta_speed = 4.70;
    float pass_weight = -47.64;
    float touch_weight = 14.07;
    float pattern_weight = 0.02;
    float connect_weight2 = -22.16;
    float cx_point_weight = 0.82;
    float corner_point_weight = -3.42;//58


    //con_weight = 100;
    uint64_t empty =  ~(board_w | board_b)& 0xFFFFFFFFFFFFFFFF;
    int turn = 64 - bit_count(empty);
    float alpha = nmin(nmax(0,(turn -alpha_start) / alpha_speed),1);
    float beta = nmin(nmax(0,(turn -beta_start) / beta_speed),1);

    float spread_score = 0,open_stones_score = 0,connected_pairs_score = 0;
    float b_snum = bit_count(board_b);
    float w_snum = bit_count(board_w);
    float connect_w_num,connect_b_num,open_w_num,open_b_num,touch_w2b,touch_b2w,touch_other;
    find_connected_open_stone(board_w,board_b,empty,&connect_w_num,&open_w_num,&touch_w2b);
    find_connected_open_stone(board_b,board_w,empty,&connect_b_num,&open_b_num,&touch_b2w);
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
void insertion_sort_desc(MoveOrder *arr, int n) {
    for (int i = 1; i < n; i++) {
        MoveOrder key = arr[i];
        int j = i - 1;
        while (j >= 0 && arr[j].score < key.score) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}
void insertion_sort_asc(MoveOrder *arr, int n) {
    for (int i = 1; i < n; i++) {
        MoveOrder key = arr[i];
        int j = i - 1;
        while (j >= 0 && arr[j].score > key.score) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
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
#define MAX_DEPTH 64

RowCol killer_moves[3][MAX_DEPTH];
void add_killer_move(RowCol move, int depth) {
    if (killer_moves[0][depth].row != move.row || killer_moves[0][depth].col != move.col) {
        killer_moves[2][depth] = killer_moves[1][depth];
        killer_moves[1][depth] = killer_moves[0][depth];
        killer_moves[0][depth] = move;
    }
}
void prioritize_killer_moves(MoveOrder *move_order_list, int size, int depth) {
    for (int k = 0; k < 3; ++k) {
        RowCol killer = killer_moves[k][depth];
        for (int i = 0; i < size; ++i) {
            if (move_order_list[i].move.row == killer.row &&
                move_order_list[i].move.col == killer.col) {
                // 先頭に移動
                MoveOrder tmp = move_order_list[i];
                for (int j = i; j > 0; --j)
                    move_order_list[j] = move_order_list[j - 1];
                move_order_list[0] = tmp;
                break;
            }
        }
    }
}
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
        if (*score > 999999) *score = 999999;
        if (*score < -999999) *score = -999999;
        //printf("score_ha:%f\n",*score);
        fflush(stdout);
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
            //printf("start_flip_TRUE\n");
            identify_flip_stone("white",&new_board_w,&new_board_b,move,1,flip_list,&flip_list_size);
            RowCol result_act2;
            float result_score2;
            /* printf(">>> call:%d",call_count);
            printf(">>> Trying move (%d, %d) at depth=%d\n", move.row, move.col, depth);
            printf(">>> New W: 0x%016llX\n", new_board_w);
            printf(">>> New B: 0x%016llX\n", new_board_b); */
            //minimax(new_board_w,new_board_b,0,alpha,beta,FALSE,true_pass,false_pass,&result_act2,&result_score2);
            int white_score=0,black_score=0;
            eval_bitboard_score(new_board_w,new_board_b,&white_score,&black_score);
            float board_score = white_score - black_score*1.5f;
            move_order_list[i] = (MoveOrder){board_score,move,new_board_w,new_board_b};
        }
        //qsort(move_order_list, move_oder_list_size, sizeof(MoveOrder), compare_move_order_desc);//maximizing_player＝=Falseでは昇順に
        insertion_sort_desc(move_order_list, move_oder_list_size);
        prioritize_killer_moves(move_order_list, move_oder_list_size, depth);
        for(int i=0;i<legal_size;i++){
            RowCol move = move_order_list[i].move;
            uint64_t new_board_w = move_order_list[i].newBoardW;
            uint64_t new_board_b = move_order_list[i].newBoardB;

            float eval;
            RowCol act2;
            
            minimax(new_board_w,new_board_b,depth-1,alpha,beta,FALSE,true_pass,false_pass,&act2,&eval);
            //printf("return_eval_TRUE\n");
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
        if (*score > 999999) *score = 999999;
        if (*score < -999999) *score = -999999;
        //printf("αβ_return_eval_TRUE\n");
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
            //rintf("start_flip_FALSE\n");
            identify_flip_stone("black",&new_board_w,&new_board_b,move,1,flip_list,&flip_list_size);
            RowCol result_act2;
            float result_score2;
            //minimax(new_board_w,new_board_b,0,alpha,beta,TRUE,true_pass,false_pass,&result_act2,&result_score2);
            int white_score=0,black_score=0;
            eval_bitboard_score(new_board_w,new_board_b,&white_score,&black_score);

            float board_score = white_score - black_score*1.5f;
            //printf("return_eval_FALSE\n");
            move_order_list[i] = (MoveOrder){board_score,move,new_board_w,new_board_b};
        }
        //qsort(move_order_list, move_oder_list_size, sizeof(MoveOrder), compare_move_order_asc);
        insertion_sort_asc(move_order_list, move_oder_list_size);
        prioritize_killer_moves(move_order_list, move_oder_list_size, depth);
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
        if (*score > 999999) *score = 999999;
        if (*score < -999999) *score = -999999;
        //printf("αβ_return_eval_FALSE\n");
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
/* #define RAND_R_CUSTOM_MAX 0x7fffffff
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
void progress_game_random_simple(int random_color,int *winner,float *diff_stone){//1が先手(黒)
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
            minimax(current_b,current_w,5,-INFINITY,INFINITY,TRUE,0,0,&act,&score);
        }else if(current_color==1&&random_color==-1){
            minimax(current_w,current_b,5,-INFINITY,INFINITY,TRUE,0,0,&act,&score);
        }else{
            act = legal_list[(int)rand_float2(0, legal_list_size)];
        }
        printf("\nscore:%f",score);
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

} */

int maina(void){
    //printf("LEFT_MASK = %llu\n", LEFT_MASK&RIGHT_MASK);
    RowCol legal_list[64];
    int legal_list_size;
    //get_legal_square("black",68853694464,34628173824,legal_list,&legal_list_size);
    white = 68853694464ULL;
    black = 34628173824ULL;
    RowCol flip_list[64];
    int flip_list_size;

    /* int winner;
    float diff_stone;
    progress_game_random_simple(1,&winner,&diff_stone);
    printf("\nwinner;%d",winner);
    printf("\ndiff_disk;%f",diff_stone);
    progress_game_random_simple(-1,&winner,&diff_stone);
    printf("\nwinner;%d",winner);
    printf("\ndiff_disk;%f",diff_stone); */
    
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
    /* white = 68761356292ULL;
    black = 34829500416ULL;
    white = 18438572485976448056ULL;
    black = 7890112756392387ULL;

    printf("\n%f",evaluate_board(white,black,0,0));
    uint64_t c_w=0,c_b=0;
    get_confirmed_stones(white,black,&c_w,&c_b);
    printf("\n%llu,%llu",c_w,c_b);
    get_confirmed_stones_lite(white,black,&c_w,&c_b);
    printf("\n%llu,%llu",c_w,c_b);

    RowCol act;
    float score;
    minimax(white,black,3,-INFINITY,INFINITY,TRUE,0,0,&act,&score);
    printf("\nfinal act:(%d,%d),score:%f",act.row,act.col,score); */

    return 0;
}