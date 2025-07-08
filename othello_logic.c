#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

uint64_t white,black;
#define LEFT_MASK 0x7F7F7F7F7F7F7F7F// 左端を0にする
#define RIGHT_MASK 0xFEFEFEFEFEFEFEFE// 右端を0にする
#define safety_mask 0xFFFFFFFFFFFFFFFF

#define FALSE   0
#define TRUE    !FALSE

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

void get_legal_square(const char* player_color,uint64_t current_board_w,uint64_t current_board_b,RowCol *legal_list, int *legal_list_size){
    
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
                legal_list[*legal_list_size] = (RowCol){row, col};
                (*legal_list_size)++;
            }
            //legal_list[*legal_list_size] = (RowCol){row,col};
            //(*legal_list_size) ++;
        }
    }
    
    //legal_list = {};
    //printf("Hellow World");
    //return legal_list;
}

void identify_flip_stone(const char* player_color,uint64_t *board_w,uint64_t *board_b,const char* action,int mode,RowCol *flip_list,int *flip_list_size){
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


    convert_act_str2bit(action,&bit_row,&bit_col);
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
    if(player_color == "black"){
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
    for(int i=0;i<6;i++){//辺以外の確定石
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
            int check_list[8];
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

                if ((black_confirmed & adjust_board_b)>0){
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
                }else if ((shifted_masked == 0) || (bit_length(shifted_masked) > 64) || (white_confirmed & adjust_board_w) != 0){
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

                if ((black_confirmed & adjust_board_w)>0){
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
                }else if ((shifted_masked == 0) || (bit_length(shifted_masked) > 64) || (black_confirmed & adjust_board_b) != 0){
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

int main(void){
    //printf("LEFT_MASK = %llu\n", LEFT_MASK&RIGHT_MASK);
    RowCol legal_list[64];
    int legal_list_size;
    //get_legal_square("black",68853694464,34628173824,legal_list,&legal_list_size);
    white = 68853694464;
    black = 34628173824;
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
    white = 18446337832994930416ULL;
    black = 300274201985295ULL;
    uint64_t c_w=0,c_b=0;
    get_confirmed_stones(white,black,&c_w,&c_b);
    printf("\n%llu,%llu",c_w,c_b);
    return 0;
}