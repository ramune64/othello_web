import numpy as np
#import othello_play_cython2

class MyError(Exception):
    pass

#文字と数字を変換するよ(convert letters to numbers)
convert_l2n = {"a" : 1, "b" : 2, "c" : 3, "d" : 4, "e" : 5, "f" : 6, "g" : 7, "h" : 8}
#数字と文字を変換するよ(convert numbers to letters)
convert_n2l = ["a", "b", "c", "d", "e", "f", "g", "h"]
#白黒をそれぞれ数字に割り当てるよ(使わないかもね)
convert_color = {"white":1,"black":-1}

# 各方向を表すビットマスク
DIRECTIONS = [
    -8,  # 下
    8,   # 上
    -1,  # 右
    1,   # 左
    -9,  # 右下
    9,   # 左上
    -7,  # 左下
    7    # 右上
]

LEFT_MASK =  0b0111111101111111011111110111111101111111011111110111111101111111  # 左端を0にする
RIGHT_MASK = 0b1111111011111110111111101111111011111110111111101111111011111110  # 右端を0にする
safety_maask = 0xFFFFFFFFFFFFFFFF
DIRECTIONS2 = [
    (-8,0xFFFFFFFFFFFFFFFF),  # 下
    (8,0xFFFFFFFFFFFFFFFF),   # 上
    (-1,LEFT_MASK),  # 右
    (1,RIGHT_MASK),   # 左
    (-9,LEFT_MASK),  # 右下
    (9,RIGHT_MASK),   # 左上
    (-7,RIGHT_MASK),  # 左下
    (7,LEFT_MASK)    # 右上
]
def convert_act_str2bit(act):
    put_pos_col = convert_l2n[act[0]]-1#文字を数字に変換してインデックスの表記に合わせる
    put_pos_row = int(act[1]) - 1#数字をインデックス表記に合わせる
    bit_row = 7-put_pos_row
    bit_col = 7-put_pos_col
    return(bit_row,bit_col)
def convert_act_bit2str(act):
    bit_row = act[0]
    bit_col = act[1]
    row = 7 - bit_row
    col = 7 - bit_col
    str_act = convert_n2l[col] + str(row+1)
    return str_act 

def shift_board(board, shift_value):
    """与えられた方向にボードをシフトする"""
    if shift_value < 0:
        return board >> abs(shift_value)  # 右シフト
    elif shift_value > 0:
        return board << abs(shift_value)  # 左シフト
#石が置けるマスを判定する関数(rowとcolumnが意味上で反転してるけど動きは正しいので放置)
def get_legal_square(player_color:str,current_board_w,current_board_b) -> list:
    """
    石が置けるマスを判定する関数

    Args:
        player_color(str):判定したい石の色"white"or"black"
        current_board(ndarray):判別してほしい盤面を表す配列(8*8の2次元配列)

    Return:
        list:石が置けるマスを入れた1次元配列(無い場合は空の配列)
    """

    legal_list = []
    legal_list2 = []

    #プレイヤーの色が黒の時は白と黒を反転させるよ(処理を簡単にする為)
    if player_color == "black":
        current_board_b, current_board_w = current_board_w, current_board_b
    elif player_color == "white":
        pass
    else:
        raise MyError("whiteとblack以外の言葉が入力されています。")
    #print(current_board)
    #白い石(1)があるインデックスを探す
    #white_indexes = np.where(current_board == 1)
    empty = ~(current_board_b | current_board_w)& 0xFFFFFFFFFFFFFFFF
    legal = 0
    
    for direction,mask in DIRECTIONS2:
        
        t = shift_board(current_board_w,direction) & mask & current_board_b & 0xFFFFFFFFFFFFFFFF
        for _ in range(5):
            t |= shift_board(t,direction) & mask & current_board_b & 0xFFFFFFFFFFFFFFFF
        legal |= shift_board(t, direction) & mask & empty & 0xFFFFFFFFFFFFFFFF
        while legal:
            l_pos = legal & -legal
            index = l_pos.bit_length() - 1
            legal-=l_pos
            row1 = (index // 8)
            col1 = index % 8
            legal_list2.append((row1,col1))
    """ while current_board_w:
        position = current_board_w & -current_board_w
        index = position.bit_length() - 1
        #row = index // 8
        #col = index % 8
        current_board_w-=position
        only_white = 1 << index
        for direction in DIRECTIONS:
            b_exist = False
            if (only_white&~LEFT_MASK&0xFFFFFFFFFFFFFFFF and (direction in [1,-7,9])) or (only_white&~RIGHT_MASK&0xFFFFFFFFFFFFFFFF and (direction in [-1,7,-9])):
                continue
            shifted_white = shift_board(only_white,direction)
            if (direction in [1,-7,9]) and direction != 8:
                shifted_white_masked = shifted_white & LEFT_MASK
            elif (direction in [-1,7,-9]) and direction != -8:
                shifted_white_masked = shifted_white & RIGHT_MASK
            else:
                shifted_white_masked = shifted_white
            adjust_board = shifted_white_masked & current_board_b

            while adjust_board:
                shifted_white = shift_board(shifted_white,direction)
                b_exist = True
                if (direction in [1,-7,9]) and direction != 8:
                    shifted_white_masked = shifted_white & LEFT_MASK
                elif (direction in [-1,7,-9]) and direction != -8:
                    shifted_white_masked = shifted_white & RIGHT_MASK
                else:
                    shifted_white_masked = shifted_white
                adjust_board = shifted_white_masked & current_board_b
            if b_exist:
                if shifted_white & empty:
                    legal_pos = shifted_white&-shifted_white
                    index = legal_pos.bit_length() - 1
                    if index >= 64:continue
                    row1 = (index // 8)
                    col1 = index % 8
                    #print((row,col))
                    #print((row1,col1))
                    legal_list.append((row1,col1)) """
    #return list(set(legal_list))
    return list(set(legal_list2))

#反転する石を判別して反映する関数dayo
def identify_flip_stone(player_color:str,current_board_w,current_board_b,action:str,mode:int=0) -> any:
    """
    反転する石を判別して反映する関数

    Args:
        player_color(str):判定したい石の色"white"or"black"
        current_board(np.ndarray):判別してほしい盤面を表す配列(8*8の2次元配列)
        action(str):石を置く場所(a1~h8)の記述方式
        mode(int=0):0なら変更が反映された盤面の配列が返り値となる
        mode(int=1):1なら裏返す石の場所が入った配列が返り値となる


    Returns:
        ndarray:渡された盤面とアクションを元に石の反転を反映させて盤面を返す(mode=0)
        or
        list:反転する石の場所が入っている(mode=1)
        or
        str:置けない位置に石を置こうとした時は"wrong"と返す(mode=0,1)
    """
    if player_color == "black":
        current_board_b, current_board_w = current_board_w, current_board_b
    elif player_color == "white":
        pass
    else:
        raise MyError("whiteとblack以外の言葉が入力されています。")
    
    if len(action) != 2:
        print(action)
        return "wrong2"
    
    legal_squares = get_legal_square("white",current_board_w,current_board_b)#現状石が置ける場所を確認
    bit_row,bit_col = convert_act_str2bit(action)
    if (bit_row,bit_col) in legal_squares:pass
    else:#石が置けない場所に石を置こうとしていたら
        return "wrong"
    index = bit_row*8 + bit_col
    only_white = 1 << index
    flip_list = []
    flip_list2 = []
    flips = 0
    for direction,mask in DIRECTIONS2:
        flip_candidate = 0
        t = shift_board(only_white,direction) & mask & current_board_b & 0xFFFFFFFFFFFFFFFF
        flip_candidate |= t
        while t:
            next_shifted = shift_board(t, direction) & mask & 0xFFFFFFFFFFFFFFFF
            if next_shifted & current_board_w:
                flips |= flip_candidate
                break
            t = next_shifted & current_board_b
            flip_candidate |= t
    current_board_b ^= flips
    current_board_w |= flips
    current_board_w |= only_white
    """ for direction in DIRECTIONS:
            black_list = []
            b_exist = False
            if (only_white&~LEFT_MASK&0xFFFFFFFFFFFFFFFF and (direction in [1,-7,9])) or (only_white&~RIGHT_MASK&0xFFFFFFFFFFFFFFFF and (direction in [-1,7,-9])):
                continue
            shifted_white = shift_board(only_white,direction)
            if (direction in [1,-7,9]) and direction != 8:
                shifted_white_masked = shifted_white & LEFT_MASK
            elif (direction in [-1,7,-9]) and direction != -8:
                shifted_white_masked = shifted_white & RIGHT_MASK
            else:
                shifted_white_masked = shifted_white
            adjust_board = shifted_white_masked & current_board_b
            black_list.append(shifted_white)
            while adjust_board:
                shifted_white = shift_board(shifted_white,direction)
                b_exist = True
                if (direction in [1,-7,9]) and direction != 8:
                    shifted_white_masked = shifted_white & LEFT_MASK
                elif (direction in [-1,7,-9]) and direction != -8:
                    shifted_white_masked = shifted_white & RIGHT_MASK
                else:
                    shifted_white_masked = shifted_white
                adjust_board = shifted_white_masked & current_board_b
                black_list.append(shifted_white)#このままだと一番最後の要素は白色の石
            if b_exist:
                if shifted_white&current_board_w:
                    for black in black_list[:-1]:
                        black_pos = black&-black
                        index = black_pos.bit_length() - 1
                        row1 = (index // 8)
                        col1 = index % 8
                        flip_list.append((row1,col1)) """
    if mode == 0 or mode == 2:#盤面を更新
        """ flip_bits = 0
        for flip in flip_list:
            index = flip[0]*8 + flip[1]
            flip_bits |= 1<<index
        current_board_b ^= flip_bits
        current_board_w |= flip_bits
        current_board_w |= only_white """
        if player_color == "black":
            current_board_b, current_board_w = current_board_w, current_board_b
        if mode == 0:
            return current_board_w,current_board_b
        elif mode == 2:
            while flips:
                f_pos = flips & -flips
                index = f_pos.bit_length() - 1
                flips-=f_pos
                row1 = (index // 8)
                col1 = index % 8
                flip_list2.append((row1,col1))
            return current_board_w,current_board_b,flip_list2
        while flips:
            f_pos = flips & -flips
            index = f_pos.bit_length() - 1
            flips-=f_pos
            row1 = (index // 8)
            col1 = index % 8
            flip_list2.append((row1,col1))
    return flip_list2


def is_terminal(board_w,board_b):
    #if get_legal_square("white",board) == [] and get_legal_square("black",board.copy()*-1) == []:
    if (board_w | board_b).bit_count() == 64:
        return True
    else:
        return False

def is_within_bounds(x, y, n=8):
    """盤面の範囲内かどうかをチェック"""
    return 0 <= x < n and 0 <= y < n

def get_color_direction_color(board, x, y, dx, dy, last_color):
    n = len(board)
    x += dx
    y += dy
    if is_within_bounds(x, y, n):
        return board[x,y]
    else:
        return "akan"

def get_confirmed_stones(board_w,board_b,mode=0,last_con_w=0,last_con_b=0):
    """盤面における確定石の枚数を求める関数"""
    #n = len(board)  # 盤面のサイズ (8x8)
    black_confirmed = 0  # 黒石の確定石を記録する数
    white_confirmed = 0  # 白石の確定石を記録する数
    #print(bitboard_to_numpy(last_con_w,last_con_b))
    confirmed_all = white_confirmed | black_confirmed
    corners = [(0,0),(0,7),(7,0),(7,7)]
    queue = []
    empty = ~(board_b | board_w)& 0xFFFFFFFFFFFFFFFF
    for row,col in corners:
        index = row*8 + col
        corner_bit = 1 << index
        #print(index)
        
        if corner_bit&board_w:
            confirmed_all |= corner_bit
            white_confirmed |= corner_bit
            queue.append((row,col,1))
            #print("white")
        elif corner_bit&board_b:
            confirmed_all |= corner_bit
            black_confirmed |= corner_bit
            queue.append((row,col,-1))
            #print("black")
    #print(bitboard_to_numpy(white_confirmed,black_confirmed))
    #print("queue",queue)
    # 端の石（隅や辺）の位置をキューに追加
    #directions = [(-1, 0), (0, -1), (0, 1), (1, 0)]
    #directions2 = [(1,1),(1,-1),(-1,1),(-1,-1)]
    #queue = []
    # 確定石を確認する
    #print("queue",queue)
    for s in queue:#垂直方向の探索
        row, col ,origin_color = s
        #print("startpos",(x,y))
        for direction in DIRECTIONS[0:4]:
            #print("dir",(dx,dy))
            last_color = origin_color
            index = row*8 + col
            now_bit = 1 << index
            
            while True:
                #result = get_color_direction_color(board,nx,ny,dx,dy,last_color)
                if (now_bit&~LEFT_MASK&0xFFFFFFFFFFFFFFFF and (direction in [1,-7,9])) or (now_bit&~RIGHT_MASK&0xFFFFFFFFFFFFFFFF and (direction in [-1,7,-9])):
                    break
                shifted = shift_board(now_bit,direction)
                """ if  (confirmed_all & now_bit) != 0:
                    continue """
                """ if (direction in [1,-7,9]) and direction != 8:
                    shifted_masked = shifted & LEFT_MASK
                elif direction < 0 and direction != -8:
                    shifted_masked = shifted & RIGHT_MASK
                else:
                    shifted_masked = shifted """
                adjust_board_w = shifted & board_w
                adjust_board_b = shifted & board_b
                #print(last_color,result)
                now_bit = shifted
                if adjust_board_w and last_color == 1:
                    #print("atta!",(nx,ny))
                    confirmed_all |= shifted
                    white_confirmed |= shifted
                    last_color = 1
                elif adjust_board_b and last_color == -1:
                    confirmed_all |= shifted
                    black_confirmed |= shifted
                    last_color = -1
                else:
                    break
    #両角とそこに挟まれる辺の色が異なる場合、辺を確定石に。
    for i in range(len(m_c_list)):
        
        #print(bitboard_to_numpy(m_c_list[i],0))
        #print(bitboard_to_numpy(m_e_list[i],0))
        white_c = board_w & m_c_list[i]
        black_c = board_b & m_c_list[i]
        black_e = board_b & m_e_list[i]
        white_e = board_w & m_e_list[i]
        if (white_c|black_c|black_e|white_e).bit_count() == 8:
            black_e = board_b & m_e_list[i]
            white_e = board_w & m_e_list[i]
            black_confirmed |= black_e
            confirmed_all |= black_e
            white_confirmed |= white_e
            confirmed_all |= white_e
                
        
    #print(queue)
    for s in queue:
        row, col ,origin_color = s
        for direction in DIRECTIONS[4:]:
            last_color = origin_color
            index = row*8 + col
            now_bit = 1 << index
            while True:
                if (now_bit&~LEFT_MASK&0xFFFFFFFFFFFFFFFF and (direction in [1,-7,9])) or (now_bit&~RIGHT_MASK&0xFFFFFFFFFFFFFFFF and (direction in [-1,7,-9])):
                    break
                shifted = shift_board(now_bit,direction)
                """ if  (confirmed_all & shifted) != 0:
                    continue """
                """ if direction > 0 and direction != 8:
                    shifted_masked = shifted & RIGHT_MASK
                elif direction < 0 and direction != -8:
                    shifted_masked = shifted & LEFT_MASK
                else:
                    shifted_masked = shifted """
                adjust_board_w = shifted & board_w
                adjust_board_b = shifted & board_b
                now_bit = shifted
                if adjust_board_w or adjust_board_b:
                    if adjust_board_w:
                        if white_confirmed & adjust_board_w:
                            break
                        last_color = 1
                    if adjust_board_b:
                        if black_confirmed & adjust_board_b:
                            break
                        last_color = -1
                    #print("naname,arimasu")
                    check_list = []#ここに1,2,3,4全てそろえばOK
                    for direction2 in DIRECTIONS:
                        #print("dir",(d2x,d2y))
                        now_bit2 = now_bit
                        last_color2 = last_color
                        #print("startpos",(nx,ny))
                        while True:
                            #print("ココが頭だ")
                            #result2 = get_color_direction_color(board,n2x,n2y,d2x,d2y,last_color2)
                            shifted2 = shift_board(now_bit2,direction2)
                            adjust_board_w2 = shifted2 & board_w
                            adjust_board_b2 = shifted2 & board_b
                            now_bit2 = shifted2
                            #n2x,n2y = n2x + d2x, n2y + d2y
                            #print(last_color2,result2)
                            #print(last_color,now_bit2.bit_length(),now_bit.bit_length())
                            if adjust_board_w2 or adjust_board_b2:#シフトした先に何かしらの石がある場合
                                if last_color2 == 1:
                                    if white_confirmed&adjust_board_w2:#そのマスが白色の確定石なら
                                        if abs(direction2) == 8:#上下方向の移動なら
                                            check_list.append(2)
                                        elif abs(direction2) == 1:
                                            #print("種類1")
                                            check_list.append(1)
                                        elif direction2 == 7 or direction2 == -7:#右上左下方向の移動なら
                                            #print("種類3")
                                            check_list.append(3)
                                        elif direction2 == -9 or direction2 == 9:#右下左上方向の移動なら
                                            #print(direction2)
                                            #print("種類4_1")
                                            check_list.append(4)
                                        last_color2 = 1
                                        break
                                    else:
                                        last_color2 = last_color
                                        break
                                        #print("ここ、確定石。")
                                else:
                                    if black_confirmed&adjust_board_b2:#そのマスが黒色の確定石なら
                                        if abs(direction2) == 8:#上下方向の移動なら
                                            check_list.append(2)
                                        elif abs(direction2) == 1:
                                            #print("種類1")
                                            check_list.append(1)
                                        elif direction2 == 7 or direction2 == -7:#右上左下方向の移動なら
                                            #print("種類3")
                                            check_list.append(3)
                                        elif direction2 == -9 or direction2 == 9:#右下左上方向の移動なら
                                            #print("種類4")
                                            check_list.append(4)
                                        #print("ここ、確定石。")
                                        last_color2 = -1
                                        break
                                    else:
                                        last_color2 = last_color
                                        break
                                                                                #右移動                                                 #左移動
                            elif (not shifted2) or shifted2.bit_length() > 64 or ((direction2 in [-1,7,-9]) and shifted2&~LEFT_MASK&0xFFFFFFFFFFFFFFFF) or ((direction2 in [1,-7,9]) and shifted2&~RIGHT_MASK&0xFFFFFFFFFFFFFFFF):
                                #print("ココ壁っすわ")
                                if abs(direction2) == 8:#上下方向の移動なら
                                    #print(direction2)
                                    check_list.append(2)
                                elif abs(direction2) == 1:
                                    #print("種類1")
                                    #print(direction2)
                                    check_list.append(1)
                                elif direction2 == 7 or direction2 == -7:#右上左下方向の移動なら
                                    #print("種類3")
                                    check_list.append(3)
                                elif direction2 == -9 or direction2 == 9:#右下左上方向の移動なら
                                    check_list.append(4)
                                    #print(direction2)
                                    #print("種類4")
                                break
                            else:
                                break
                    #print(check_list)
                    if (1 in check_list) and (2 in check_list) and (3 in check_list) and (4 in check_list):
                        #print("新たな確定石、素晴らしい。")
                        if adjust_board_w:
                        #print("atta!",(nx,ny))
                            confirmed_all |= adjust_board_w
                            white_confirmed |= adjust_board_w
                            last_color = 1
                        else:
                            confirmed_all |= adjust_board_b
                            black_confirmed |= adjust_board_b
                            last_color = -1
                else:
                    break
    #return white_confirmed,black_confirmed
    
    


    #ここからは他の場所の確定石を数えていきます

    for k in range(2):
        all_white = []
        get_all_white = board_w
        while get_all_white:
                position = get_all_white & -get_all_white
                get_all_white-=position
                index = position.bit_length() - 1
                #row = index // 8
                #col = index % 8
                only_white = 1 << index
                if not only_white&white_confirmed:
                    all_white.append(only_white)
                #(列(上から),行(左から))のタプルの配列に変換
                #all_white = list(zip(all_white[0], all_white[1]))
        #print(all_white)

        get_all_black = board_b
        all_black = []
        while get_all_black:
                position = get_all_black & -get_all_black
                index = position.bit_length() - 1
                get_all_black-=position
                row = index // 8
                col = index % 8
                #print(row,col)
                only_black = 1 << index
                if not only_black&black_confirmed:
                    all_black.append(only_black)
                #(列(上から),行(左から))のタプルの配列に変換
                #all_white = list(zip(all_white[0], all_white[1]))
        for only_white in all_white:
                if not only_white & white_confirmed:#この石がまだ確定石判定を受けていなかったら
                    #print("マダ")
                    #index = only_black.bit_length() - 1
                    #row = index // 8
                    #col = index % 8
                    #print(row,col)
                    check_list = []
                    now_bit = only_white
                    for direction in DIRECTIONS:
                        #nx,ny = wx,wy
                        #result = get_color_direction_color(board,nx,ny,dx,dy,0)
                        now_bit = only_white
                        shifted = shift_board(now_bit,direction)
                        if (direction in [-1,7,-9]) and direction != 8:
                            shifted_masked = shifted & LEFT_MASK
                        elif (direction in [1,-7,9]) and direction != -8:
                            shifted_masked = shifted & RIGHT_MASK
                        else:
                            shifted_masked = shifted
                        adjust_board_w = shifted_masked & board_w
                        adjust_board_b = shifted_masked & board_b
                        """ if abs(direction) == 9:
                            print(direction)
                            print(bitboard_to_numpy(0,now_bit))
                            print(bitboard_to_numpy(0,shifted))
                            print(shifted&~LEFT_MASK) """
                        if black_confirmed & adjust_board_b:#その方向が違う色の確定石なら
                            #print("w")
                            #print(direction)
                            if abs(direction) == 8:#上下方向の移動なら
                                #print(6)
                                check_list.append(6)
                            elif abs(direction) == 1:
                                check_list.append(5)
                            elif direction == -7 or direction == 7:#右上左下方向の移動なら
                                #print("種類3")
                                check_list.append(7)
                            elif direction == -9 or direction == 9:#右下左上方向の移動なら
                                check_list.append(8)
                        elif (shifted_masked == 0) or (shifted_masked.bit_length() > 64) or (white_confirmed & adjust_board_w) != 0 :#その方向が壁か同じ色の確定石なら
                            """ print("壁or同じ色の確定石")
                            print("同じ色の確定石：",white_confirmed & adjust_board_w)
                            print(direction)
                            if direction == 1:
                                print(bitboard_to_numpy(shifted&~RIGHT_MASK&0xFFFFFFFFFFFFFFFF,0))
                                print(shifted.bit_length()) """
                            #print(black_confirmed & adjust_board_b)
                            #print("b")
                            #print(direction)
                            
                            if abs(direction) == 8:#上下方向の移動なら
                                check_list.append(2)
                            elif abs(direction) == 1:
                                check_list.append(1)
                            elif direction == -7 or direction == 7:#右上左下方向の移動なら
                                #print("種類3")
                                check_list.append(3)
                            elif direction == -9 or direction == 9:#右下左上方向の移動なら
                                check_list.append(4)
                        #else:
                            #print("u")
                            #print(direction)
                    #print("white")
                    #print(check_list)
                    #リストを変換するよ
                    #print(check_list)
                    for num in [5,6,7,8]:
                        if check_list.count(num) == 2:
                            check_list = [x for x in check_list if x != num]#その値を全て削除
                            check_list.append(num-4)
                    
                    if (1 in check_list) and (2 in check_list) and (3 in check_list) and (4 in check_list):
                        confirmed_all |= only_white
                        white_confirmed |= only_white

        
                

        #print(all_black)
        for only_black in all_black:
                if not only_black & black_confirmed:#この石がまだ確定石判定を受けていなかったら
                    #index = only_black.bit_length() - 1
                    #row = index // 8
                    #col = index % 8
                    #print(row,col)
                    check_list = []
                    now_bit = only_black
                    for direction in DIRECTIONS:
                        #nx,ny = wx,wy
                        #result = get_color_direction_color(board,nx,ny,dx,dy,0)
                        now_bit = only_black
                        shifted = shift_board(now_bit,direction)

                        if (direction in [-1,7,-9]) and direction != 8:
                            shifted_masked = shifted & LEFT_MASK
                        elif (direction in [1,-7,9]) and direction != -8:
                            shifted_masked = shifted & RIGHT_MASK
                        else:
                            shifted_masked = shifted
                        adjust_board_w = shifted_masked & board_w
                        adjust_board_b = shifted_masked & board_b
                        
                        """ print(direction)
                        print(bitboard_to_numpy(0,shifted))
                        print(bitboard_to_numpy(0,shifted_masked)) """
                            #print(shifted&~LEFT_MASK)
                        if white_confirmed & adjust_board_w:#その方向が違う色の確定石なら
                            #print("w")
                            #print(direction)
                            if abs(direction) == 8:#上下方向の移動なら
                                #print(6)
                                check_list.append(6)
                            elif abs(direction) == 1:
                                check_list.append(5)
                            elif direction == -7 or direction == 7:#右上左下方向の移動なら
                                #print("種類3")
                                check_list.append(7)
                            elif direction == -9 or direction == 9:#右下左上方向の移動なら
                                check_list.append(8)
                        elif (shifted_masked == 0) or shifted_masked.bit_length() > 64  or (black_confirmed & adjust_board_b) :#その方向が壁か同じ色の確定石なら
                            #print(black_confirmed & adjust_board_b)
                            #print("b")
                            #print(direction)
                            
                            if abs(direction) == 8:#上下方向の移動なら
                                check_list.append(2)
                            elif abs(direction) == 1:
                                check_list.append(1)
                            elif direction == -7 or direction == 7:#右上左下方向の移動なら
                                #print("種類3")
                                check_list.append(3)
                            elif direction == -9 or direction == 9:#右下左上方向の移動なら
                                check_list.append(4)
                        #else:
                            #print("u")
                            #print(direction)
                    #print(check_list)
                    #リストを変換するよ
                    for num in [5,6,7,8]:
                        if check_list.count(num) == 2:
                            check_list = [x for x in check_list if x != num]#その値を全て削除
                            check_list.append(num-4)
                    
                    if (1 in check_list) and (2 in check_list) and (3 in check_list) and (4 in check_list):
                        confirmed_all |= only_black
                        black_confirmed |= only_black
        #return white_confirmed,black_confirmed
        #2週目(逆順)
        all_white = []
        get_all_white = board_w
        while get_all_white:
                position = get_all_white & -get_all_white
                get_all_white-=position
                index = position.bit_length() - 1
                #row = index // 8
                #col = index % 8
                only_white = 1 << index
                if not only_white&white_confirmed:
                    all_white.append(only_white)
                #(列(上から),行(左から))のタプルの配列に変換
                #all_white = list(zip(all_white[0], all_white[1]))
        #print(all_white)

        get_all_black = board_b
        all_black = []
        while get_all_black:
                position = get_all_black & -get_all_black
                index = position.bit_length() - 1
                get_all_black-=position
                row = index // 8
                col = index % 8
                #print(row,col)
                only_black = 1 << index
                if not only_black&black_confirmed:
                    all_black.append(only_black)
                #(列(上から),行(左から))のタプルの配列に変換
                #all_white = list(zip(all_white[0], all_white[1]))
        for only_white in reversed(all_white):
                if not only_white & white_confirmed:#この石がまだ確定石判定を受けていなかったら
                    #print("マダ")
                    #index = only_black.bit_length() - 1
                    #row = index // 8
                    #col = index % 8
                    #print(row,col)
                    check_list = []
                    now_bit = only_white
                    for direction in DIRECTIONS:
                        #nx,ny = wx,wy
                        #result = get_color_direction_color(board,nx,ny,dx,dy,0)
                        now_bit = only_white
                        shifted = shift_board(now_bit,direction)
                        if (direction in [-1,7,-9]) and direction != 8:
                            shifted_masked = shifted & LEFT_MASK
                        elif (direction in [1,-7,9]) and direction != -8:
                            shifted_masked = shifted & RIGHT_MASK
                        else:
                            shifted_masked = shifted
                        adjust_board_w = shifted_masked & board_w
                        adjust_board_b = shifted_masked & board_b
                        """ if abs(direction) == 9:
                            print(direction)
                            print(bitboard_to_numpy(0,now_bit))
                            print(bitboard_to_numpy(0,shifted))
                            print(shifted&~LEFT_MASK) """
                        if black_confirmed & adjust_board_b:#その方向が違う色の確定石なら
                            #print("w")
                            #print(direction)
                            if abs(direction) == 8:#上下方向の移動なら
                                #print(6)
                                check_list.append(6)
                            elif abs(direction) == 1:
                                check_list.append(5)
                            elif direction == -7 or direction == 7:#右上左下方向の移動なら
                                #print("種類3")
                                check_list.append(7)
                            elif direction == -9 or direction == 9:#右下左上方向の移動なら
                                check_list.append(8)
                        elif (shifted_masked == 0) or (shifted_masked.bit_length() > 64) or (white_confirmed & adjust_board_w) != 0 :#その方向が壁か同じ色の確定石なら
                            """ print("壁or同じ色の確定石")
                            print("同じ色の確定石：",white_confirmed & adjust_board_w)
                            print(direction)
                            if direction == 1:
                                print(bitboard_to_numpy(shifted&~RIGHT_MASK&0xFFFFFFFFFFFFFFFF,0))
                                print(shifted.bit_length()) """
                            #print(black_confirmed & adjust_board_b)
                            #print("b")
                            #print(direction)
                            
                            if abs(direction) == 8:#上下方向の移動なら
                                check_list.append(2)
                            elif abs(direction) == 1:
                                check_list.append(1)
                            elif direction == -7 or direction == 7:#右上左下方向の移動なら
                                #print("種類3")
                                check_list.append(3)
                            elif direction == -9 or direction == 9:#右下左上方向の移動なら
                                check_list.append(4)
                        #else:
                            #print("u")
                            #print(direction)
                    #print("white")
                    #print(check_list)
                    #リストを変換するよ
                    #print(check_list)
                    for num in [5,6,7,8]:
                        if check_list.count(num) == 2:
                            check_list = [x for x in check_list if x != num]#その値を全て削除
                            check_list.append(num-4)
                    
                    if (1 in check_list) and (2 in check_list) and (3 in check_list) and (4 in check_list):
                        confirmed_all |= only_white
                        white_confirmed |= only_white
        for only_black in reversed(all_black):
                if not only_black & black_confirmed:#この石がまだ確定石判定を受けていなかったら
                    #index = only_black.bit_length() - 1
                    #row = index // 8
                    #col = index % 8
                    #print(row,col)
                    check_list = []
                    now_bit = only_black
                    for direction in DIRECTIONS:
                        #nx,ny = wx,wy
                        #result = get_color_direction_color(board,nx,ny,dx,dy,0)
                        now_bit = only_black
                        shifted = shift_board(now_bit,direction)

                        if (direction in [-1,7,-9]) and direction != 8:
                            shifted_masked = shifted & LEFT_MASK
                        elif (direction in [1,-7,9]) and direction != -8:
                            shifted_masked = shifted & RIGHT_MASK
                        else:
                            shifted_masked = shifted
                        adjust_board_w = shifted_masked & board_w
                        adjust_board_b = shifted_masked & board_b
                        
                        """ print(direction)
                        print(bitboard_to_numpy(0,shifted))
                        print(bitboard_to_numpy(0,shifted_masked)) """
                            #print(shifted&~LEFT_MASK)
                        if white_confirmed & adjust_board_w:#その方向が違う色の確定石なら
                            #print("w")
                            #print(direction)
                            if abs(direction) == 8:#上下方向の移動なら
                                #print(6)
                                check_list.append(6)
                            elif abs(direction) == 1:
                                check_list.append(5)
                            elif direction == -7 or direction == 7:#右上左下方向の移動なら
                                #print("種類3")
                                check_list.append(7)
                            elif direction == -9 or direction == 9:#右下左上方向の移動なら
                                check_list.append(8)
                        elif (shifted_masked == 0) or shifted_masked.bit_length() > 64  or (black_confirmed & adjust_board_b) :#その方向が壁か同じ色の確定石なら
                            #print(black_confirmed & adjust_board_b)
                            #print("b")
                            #print(direction)
                            
                            if abs(direction) == 8:#上下方向の移動なら
                                check_list.append(2)
                            elif abs(direction) == 1:
                                check_list.append(1)
                            elif direction == -7 or direction == 7:#右上左下方向の移動なら
                                #print("種類3")
                                check_list.append(3)
                            elif direction == -9 or direction == 9:#右下左上方向の移動なら
                                check_list.append(4)
                        #else:
                            #print("u")
                            #print(direction)
                    #print(check_list)
                    #リストを変換するよ
                    for num in [5,6,7,8]:
                        if check_list.count(num) == 2:
                            check_list = [x for x in check_list if x != num]#その値を全て削除
                            check_list.append(num-4)
                    
                    if (1 in check_list) and (2 in check_list) and (3 in check_list) and (4 in check_list):
                        confirmed_all |= only_black
                        black_confirmed |= only_black
    return white_confirmed,black_confirmed


scores = np.array([
    [ 30, -12,   0,  -1,  -1,   0, -12, 30],
    [-12, -20,  -3,  -3,  -3,  -3, -20,-12],
    [  0,  -3,   0,  -1,  -1,   0,  -3,  0],
    [ -1,  -3,  -1,  -1,  -1,  -1,  -3, -1],
    [ -1,  -3,  -1,  -1,  -1,  -1,  -3, -1],
    [  0,  -3,   0,  -1,  -1,   0,  -3,  0],
    [-12, -20,  -3,  -3,  -3,  -3, -20,-12],
    [ 30, -12,   0,  -1,  -1,   0, -12, 30]
])
score_flat = scores.flatten().tolist()

cx_zone = [(0,1),(1,1),(1,0),(0,6),(1,7),(1,6),(6,0),(6,1),(7,1),(6,7),(7,6),(6,6)]
c_zone = [(0,1),(0,6),(1,7),(6,7),(7,1),(7,6),(1,0),(6,0)]
cx_zone0 = [(0,1),(1,1),(1,0)]
cx_zone1 = [(0,6),(1,7),(1,6)]
cx_zone2 = [(6,0),(6,1),(7,1)]
cx_zone3 = [(6,7),(7,6),(6,6)]

corner = [(0,0),(0,7),(7,7),(7,0)]
m1_e = 0x7E
m1_c = 0x81
m3_e = 0x7E00000000000000
m3_c = 0x8100000000000000
m4_e = 0x0001010101010100
m4_c = 0x0100000000000001
m2_e = 0x0080808080808000
m2_c = 0x8000000000000080
m_e_list = [m1_e,m2_e,m3_e,m4_e]
m_c_list = [m1_c,m2_c,m3_c,m4_c]
def eval_bitboard_score(board_b, board_w, score_flat):
    black_score = 0
    white_score = 0
    for i in range(64):
        mask = 1 << i
        if board_b & mask:
            black_score += score_flat[i]
        elif board_w & mask:
            white_score += score_flat[i]
    return black_score, white_score
def evaluate_board(board_w,board_b,true_pass=0,false_pass=0):
    con_weight = 80
    empty = ~(board_w | board_b)& 0xFFFFFFFFFFFFFFFF
    turn = 64 - empty.bit_count()
    #print("turn:",turn)
    alpha = min(max(0, (turn - 35) / 20),1)  # 40手目以降、徐々に石の数を重視
    num_w = get_legal_square("white",board_w,board_b)
    num_b = get_legal_square("black",board_w,board_b)
    #legal_base_point = 15
    #w_legal_point = legal_base_point*len(num_w)
    #b_legal_point = legal_base_point*len(num_b)
    """ for w in num_w:
        idx = 63-(w[0]*8+w[1])
        w_legal_point += score_flat[idx]
    for b in num_b:
        idx = 63-(b[0]*8+b[1])
        b_legal_point += score_flat[idx]
    total_legal_point = w_legal_point - b_legal_point """
    #print(total_legal_point)
    """ if board[0,0] != 1:
        w_cx0 = sum(x in cx_zone0 for x in num_w)
    elif board[0,0] != -1:
        b_cx0 = sum(x in cx_zone0 for x in num_b)
    if board[0,7] != 1:
        w_cx1 = sum(x in cx_zone1 for x in num_w)
    elif board[0,7] != -1:
        b_cx1 = sum(x in cx_zone1 for x in num_b)
    if board[7,0] != 1:
        w_cx2 = sum(x in cx_zone2 for x in num_w)
    elif board[7,0] != -1:
        b_cx2 = sum(x in cx_zone2 for x in num_b)
    if board[7,7] != 1:
        w_cx3 = sum(x in cx_zone3 for x in num_w)
    elif board[7,7] != -1:
        b_cx3 = sum(x in cx_zone3 for x in num_b) """
    
    w_cx = sum(x in cx_zone for x in num_w)
    
    #w_corner = sum(x in corner for x in num_w)
    b_cx = sum(x in cx_zone for x in num_b)
    zennmetu_keikoku = 0
    if board_w.bit_count() <= 2 and turn>=10:
        zennmetu_keikoku = -45
    

    #1辺全部1色かつ角が相手にとられないなら加点
    edge_point_w = 0
    edge_point_b = 0
    #print(board)
    for idx in range(0,7,2):
        dec_point = 0
        add_point = 0
        """ s = c_zone[idx]
        g = c_zone[idx+1]
        if s[0] == g[0]:
            increase_idx = 1
            increase_s = s[1]
            fix_num = s[0]#固定するインデックスの値
        else:
            increase_idx = 0
            increase_s = s[0]
            fix_num = s[1] """
        #白について実行
        w_num = 0
        b_num = 0
        w_num = (board_w&m_e_list[idx//2]).bit_count()

        b_num = (board_b&m_e_list[idx//2]).bit_count()
        """ for edge in range(6):
            #print("edge",edge)
            if increase_idx == 1:
                pos = (fix_num,increase_s+edge)
            else:
                pos = (increase_s+edge,fix_num)
            #print("pos:",pos)
            #print("color:",board[pos])
            index = pos[0]*8 + pos[1]
            only_bit = 1 << index
            if board_w&only_bit:#該当箇所が白
                w_num += 1
            else:
                break """
        #print(w_num)
        if w_num == 6:
            c = [0,0]
            m_corner = m_c_list[idx//2]
            for i in range(2):
                position = m_corner & -m_corner
                m_corner -= position
                c_index = position.bit_length() - 1
                #既に角にどちらかの石がある場合
                """ c_pos = corner[idx//2-4]
                c_index = c_pos[0]*8 + c_pos[1] """
                c_bit = 1 << c_index
                adjust_board_w = c_bit & board_w
                if not empty&c_bit:
                    if adjust_board_w:
                        c[i] = 1
                    else:
                        c[i] = -1
            #if c1 == c2:
            #    pass#減点なし
            #elif c1 != c2 and c1 != 0 and c2 != 0:
            #    pass#減点なし
            c1,c2 = c
            if c1 != c2 and ((c1 == 0 and c2 == -1) or (c1 == -1 and c2 == 0)):#角に敵の石が既にあるとき
                #減点の対象
                #print("角に敵の石が既にあるとき")
                dec_point += 10*6*con_weight* 4/5/10
            #敵の合法手が角にある場合
            if corner[idx//2-4] in num_b or corner[idx//2+1-4] in num_b:
                #print("敵の合法手が角にある場合")
                #減点の対象
                dec_point += 10*6*con_weight* 3/5/10
            else:#ない場合
                #加点の対象
                #print("加点や！")
                add_point += 10*6*con_weight* 4/5/10
            edge_point_w += add_point
            edge_point_w -= dec_point
            dec_point = 0
            add_point = 0
        if b_num == 0:
            edge_point_w += w_num*con_weight*1/20

        if b_num == 6:
            c = [0,0]
            m_corner = m_c_list[idx//2]
            for i in range(2):
                position = m_corner & -m_corner
                m_corner -= position
                c_index = position.bit_length() - 1
                #既に角にどちらかの石がある場合
                """ c_pos = corner[idx//2-4]
                c_index = c_pos[0]*8 + c_pos[1] """
                c_bit = 1 << c_index
                adjust_board_w = c_bit & board_w
                if not empty&c_bit:
                    if adjust_board_w:
                        c[i] = 1
                    else:
                        c[i] = -1
                #if c1 == c2:
                #    pass#減点なし
                #elif c1 != c2 and c1 != 0 and c2 != 0:
                #    pass#減点なし
                c1,c2 = c
                if c1 != c2 and ((c1 == 0 and c2 == 1) or (c1 == 1 and c2 == 0)):#角に敵の石が既にあるとき
                    #減点の対象
                    dec_point += 10*6*con_weight* 4/5/10
                #敵の合法手が角にある場合
                if corner[idx//2-4] in num_w or corner[idx//2+1-4] in num_w:
                    #減点の対象
                    dec_point += 10*6*con_weight* 3/5/10
                else:#ない場合
                    #加点の対象
                    add_point += 10*6*con_weight* 4/5/10
        edge_point_b += add_point - dec_point
        if w_num==0:
            edge_point_b += b_num*con_weight*1/20

    edge_point = edge_point_w - edge_point_b
    #edge_point = 0


    #b_corner = sum(x in corner for x in num_b)
    dis_num = (len(num_w)) - (len(num_b)) - w_cx + b_cx
    """ if num_b == [] and num_w != []:
    """
    #score = (1-alpha) * dis_num + (1 - alpha) * np.sum(scores * board) *2.4 + alpha * np.sum(board) *1.5 *2.4
    # 黒の評価値（black_boardの位置にある評価値を合計）
    #black_score = np.sum(score_flat * ((board_b >> np.arange(64,dtype=object)) & 1))

    # 白の評価値（white_boardの位置にある評価値を合計）
    #white_score = np.sum(score_flat * ((board_w >> np.arange(64,dtype=object)) & 1))
    black_score,white_score = eval_bitboard_score(board_b,board_w,score_flat)
    #print(white_score)
    #print(black_score)
    board_score = white_score - black_score*1.5
    b_snum = board_b.bit_count()
    w_snum = board_w.bit_count()
    lose_keikoku = 0
    if num_w == [] and num_b == [] and b_snum > w_snum:
        lose_keikoku = -100
    pass_bonus = false_pass-true_pass
    #if pass_bonus != 0:print(pass_bonus)
    """ if num_b != [] and num_w == []:
        lose_keikoku-=300 """
    #(合法手の差 + 盤面スコア)*1.1*(1-alpha)+alptha*(枚数差)*1.5
    score = (1-alpha) * (dis_num*300/100 +  board_score*200/100) + alpha * ((w_snum-b_snum)*400/100+(len(num_w)-len(num_b))*400/100)
    #print(alpha)
        #score = np.sum(board * scores)
        #print(score,np.sum(board)*1.5)
    """ else:
        board = w_board.copy() * -1
        score = sum(sum(board * scores_fin)) """
    #print("score",board)

    w_c,b_c = get_confirmed_stones(board_w,board_b)
    w_c,b_c = w_c.bit_count(),b_c.bit_count()
    con_score = (w_c - b_c)
    #print(con_score)
    """ print(board_score)
    print("con:",con_score)
    print("edge",edge_point)
    print("nomal:",score)
    print("dis:",dis_num)
    #print(alpha)
    print(zennmetu_keikoku)
    #print(score)
    #print(con_score*con_weight)
    print(edge_point) """
    
    return  (score*10 + con_score*con_weight*10 + edge_point*10)/10 + zennmetu_keikoku + lose_keikoku + pass_bonus*90*alpha

def new_board_and_eval(move,now_board,color):
    move_string = convert_n2l[move[1]]+str(move[0]+1)
    new_board = identify_flip_stone(color,now_board.copy(),move_string)
    return evaluate_board(new_board)

mask1 = 0xF0F0F0F00F0F0F0F#4*4の右下と左上のマスク
not_mask1 = ~mask1&safety_maask#動かないところ
mask2_l = 0xF0F0F0F0F0F0F0F0#左右反転用マスク(左)
mask2_r = 0x0F0F0F0F0F0F0F0F#左右反転用マスク(左)
mask3_1 = 0x0000333300003333#2*2の右下のマスク
mask3_2 = 0xCCCC0000CCCC0000#2*2の左下のマスク
not_mask3 = ~(mask3_1|mask3_2)&safety_maask
mask4_r = 0x3333333333333333#左右反転用マスク(左)
mask4_l = 0xCCCCCCCCCCCCCCCC#左右反転用マスク(左)
mask5_1 = 0x0055005500550055#1*1の右下のマスク
mask5_2 = 0xAA00AA00AA00AA00#1*1の左下のマスク
mask6_r = 0x5555555555555555#左右反転用マスク(左)
mask6_l = 0xAAAAAAAAAAAAAAAA#左右反転用マスク(左)
not_mask5 = ~(mask5_1|mask5_2)&safety_maask
def rotate90(board):
    t4 = ((board&mask1)<<36|(board&mask1)>>36)|(board&not_mask1)#4*4
    t4m = (t4&mask2_r)<<4|(t4&mask2_l)>>4#4*4左右ミラー
    t2 = (t4m&mask3_1)<<18|(t4m&mask3_2)>>18|(t4m&not_mask3)#2*2
    t2m = (t2&mask4_r)<<2|(t2&mask4_l)>>2#2*2左右ミラー
    t1 = (t2m&mask5_1)<<9|(t2m&mask5_2)>>9|(t2m&not_mask5)#1*1
    t1m = (t1&mask6_r)<<1|(t1&mask6_l)>>1#1*1左右ミラー
    return t1m


# すでに探索した盤面を保存する（トランスポジションテーブル）
#zobristhash = zobrist_hash.ZobristHash()
def minimax(board_w,board_b,depth,alpha, beta,maximizing_player,boardhash=None,other_score=False,stop_queue=None,true_pass=0,false_pass=0):
    #board1 = board.copy()
    """ if boardhash == None:
        boardhash = zobristhash.compute_hash(board_w,board_b) """
    new_boardhash = None
    #print("-------depth---------",depth)
    #print(is_terminal(board1))
    white_legal_list=get_legal_square("white",board_w,board_b)
    black_legal_list=get_legal_square("black",board_w,board_b)
    if depth == 0 or is_terminal(board_w,board_b) or (white_legal_list==[] and black_legal_list==[]):    #ここが葉ノードなら
        #print("-----葉ノード-----")
        score = evaluate_board(board_w,board_b,true_pass,false_pass)
        #if score == float("inf"):
            #print("inf")
            #print(board1)

        return score, None  # 盤面評価値を返す
    #print("board",board1)
    other_scores = {}
    if maximizing_player:
        #print("aTrue")
        legal_list=white_legal_list
        legal_list = sorted(legal_list)
        #legal_list = sorted(legal_list,key=lambda x:new_board_and_eval(x,board1.copy(),"white"),reverse=True)
        if legal_list == []:
            return minimax(board_w,board_b,depth,alpha,beta,False,boardhash,stop_queue=stop_queue,true_pass=true_pass+1,false_pass=false_pass)
        max_eval = float('-inf')
        best_move = None
        ab_breake = False
        cache_list = []
        move_oder_list = []
        for move in legal_list:
            move_string = convert_act_bit2str(move)
            new_board_w,new_board_b,flip_list = identify_flip_stone("white",board_w,board_b,move_string,mode=2)
            #new_boardhash = zobristhash.update_hash(boardhash,move[0],move[1],0,1)
            #for flip in flip_list:
            #    new_boardhash = zobristhash.update_hash(new_boardhash,flip[0],flip[1],-1,1)#ひっくり返る石の処理
            eval_d0,_ = minimax(new_board_w,new_board_b,0,alpha, beta,False,new_boardhash,stop_queue=stop_queue,true_pass=true_pass,false_pass=false_pass)
            move_oder_list.append((eval_d0,move,new_board_w,new_board_b,new_boardhash))
        move_oder_list.sort(reverse=True)
        legal_list = [(move,b_w,b_b,n_b) for _, move,b_w,b_b,n_b in move_oder_list]
        for move,new_board_w,new_board_b,new_boardhash in legal_list:
            if stop_queue != None:
                if not stop_queue.empty():
                    return None,None

            #move_string = convert_n2l[move[1]]+str(move[0]+1)
            move_string = convert_act_bit2str(move)
            #print("depth:",depth)
            #print("move:",move_string)
            #print("move",move_string)
            #print("depth",depth)
            #print("mae",board1)
            #new_board_w,new_board_b,flip_list = identify_flip_stone("white",board_w,board_b,move_string,mode=2)
            """ board_combined = bitboard_to_numpy(new_board_w, new_board_b)
            print(board_combined) """
            #print("new",new_board)
            #new_boardhash = zobristhash.update_hash(boardhash,move[0],move[1],0,1)#新たに置かれた石の処理
            #for flip in flip_list:
            #    new_boardhash = zobristhash.update_hash(new_boardhash,flip[0],flip[1],-1,1)#ひっくり返る石の処理
            #print(new_boardhash)
            #new_boardhash = zobristhash.compute_hash(new_board_w, new_board_b)
            #eval = zobristhash.get_saved_score(board_hash=new_boardhash,depth=depth-1,max_pl=False)
            #print(eval)
            eval = None
            if eval != None:
                #print(eval)
                eval2 ,_ = minimax(new_board_w,new_board_b,depth-1,alpha, beta,False,new_boardhash,stop_queue=stop_queue,true_pass=true_pass,false_pass=false_pass)
                if eval != eval2:
                    print("different")
                pass
                #print("既出や1")
            else:
                """ new_board_90_w,new_board_90_b = rotate90(new_board_w),rotate90(new_board_b)
                boardhash_90 = zobristhash.compute_hash(new_board_90_w,new_board_90_b)
                eval = zobristhash.get_saved_score(boardhash_90,depth-1,False)
                if eval is None:
                    new_board_180_w,new_board_180_b = rotate90(new_board_90_w),rotate90(new_board_90_b)
                    boardhash_180 = zobristhash.compute_hash(new_board_180_w,new_board_180_b)
                    eval = zobristhash.get_saved_score(boardhash_180,depth-1,False)
                    if eval is None:
                        new_board_270_w,new_board_270_b = rotate90(new_board_180_w),rotate90(new_board_180_b)
                        boardhash_270 = zobristhash.compute_hash(new_board_270_w,new_board_270_b)
                        eval = zobristhash.get_saved_score(boardhash_270,depth-1,False)
                        if eval is None: """
            eval ,_ = minimax(new_board_w,new_board_b,depth-1,alpha, beta,False,new_boardhash,stop_queue=stop_queue,true_pass=true_pass,false_pass=false_pass)
            cache_list.append([new_boardhash,depth-1,False,eval])
            #print("score:",eval)
            if stop_queue != None:
                if not stop_queue.empty():
                    return None,None
            if other_score:
                other_scores[move_string] = eval

                #transposition_table[boardhash] = eval
            if eval > max_eval:
                max_eval = eval
                best_move = move_string
            alpha = max(alpha, eval)  # αを更新
            if beta <= alpha:  # αβ枝刈り
                #print("α")
                ab_breake = True
                break
        #if not ab_breake:
            #for c in cache_list:
                #zobristhash.save_score(board_hash=c[0],depth=c[1],max_pl=c[2],score=c[3])
        if other_score:
            return max_eval,best_move,other_scores
        return max_eval,best_move
    else:
        #print("aFalse")
        legal_list=black_legal_list
        legal_list = sorted(legal_list)
        #legal_list = sorted(legal_list,key=lambda x:new_board_and_eval(x,board1.copy(),"black"),reverse=False)
        min_eval = float('inf')
        best_move = None
        #evals = []
        evals_c = []
        if legal_list == []:
            return minimax(board_w,board_b,depth,alpha,beta,True,boardhash,stop_queue=stop_queue,true_pass=true_pass,false_pass=false_pass+1)
        ab_breake = False
        cache_list = []
        move_oder_list = []
        for move in legal_list:
            move_string = convert_act_bit2str(move)
            new_board_w,new_board_b,flip_list = identify_flip_stone("black",board_w,board_b,move_string,mode=2)
            #new_boardhash = zobristhash.update_hash(boardhash,move[0],move[1],0,1)
            #for flip in flip_list:
                #new_boardhash = zobristhash.update_hash(new_boardhash,flip[0],flip[1],-1,1)#ひっくり返る石の処理
            eval_d0,_ = minimax(new_board_w,new_board_b,0,alpha, beta,True,new_boardhash,stop_queue=stop_queue,true_pass=true_pass,false_pass=false_pass)
            move_oder_list.append((eval_d0,move,new_board_w,new_board_b,new_boardhash))
        move_oder_list.sort()
        legal_list = [(move,b_w,b_b,n_b) for _, move,b_w,b_b,n_b in move_oder_list]
        for move,new_board_w,new_board_b,new_boardhash in legal_list:
            if stop_queue != None:
                if not stop_queue.empty():
                    return None,None
            move_string = convert_act_bit2str(move)
            #print(move_string)
            #print("move",move_string)
            #print("depth",depth)
            #print("mae",board1)
            #new_board_w,new_board_b,flip_list = identify_flip_stone("black",board_w,board_b,move_string,mode=2)
            #print(a)
            
            #print("new",new_board)
            #new_boardhash = zobristhash.update_hash(boardhash,move[0],move[1],0,-1)#新たに置かれた石の処理
            #for flip in flip_list:
            #    new_boardhash = zobristhash.update_hash(new_boardhash,flip[0],flip[1],1,-1)#ひっくり返る石の処理
            
            #new_boardhash = zobristhash.compute_hash(new_board_w, new_board_b)
            #eval = zobristhash.get_saved_score(board_hash=new_boardhash,depth=depth-1,max_pl=True)
            #print(eval)
            eval = None
            if eval != None:
                #print(eval)
                #eval2 ,_ = minimax(new_board_w,new_board_b,depth-1,alpha, beta,True,new_boardhash,stop_queue=stop_queue,true_pass=true_pass,false_pass=false_pass)
                #if eval != eval2:
                #    print(new_board_w,new_board_b)
                #    print("different2")

                pass
                #print("既出や1")
            else:
                """ new_board_90_w,new_board_90_b = rotate90(new_board_w),rotate90(new_board_b)
                boardhash_90 = zobristhash.compute_hash(new_board_90_w,new_board_90_b)
                eval = zobristhash.get_saved_score(boardhash_90,depth-1,True)
                if eval is None:
                    new_board_180_w,new_board_180_b = rotate90(new_board_90_w),rotate90(new_board_90_b)
                    boardhash_180 = zobristhash.compute_hash(new_board_180_w,new_board_180_b)
                    eval = zobristhash.get_saved_score(boardhash_180,depth-1,True)
                    if eval is None:
                        new_board_270_w,new_board_270_b = rotate90(new_board_180_w),rotate90(new_board_180_b)
                        boardhash_270 = zobristhash.compute_hash(new_board_270_w,new_board_270_b)
                        eval = zobristhash.get_saved_score(boardhash_270,depth-1,True)
                        if eval is None: """
            eval ,_ = minimax(new_board_w,new_board_b,depth-1,alpha, beta,True,new_boardhash,stop_queue=stop_queue,true_pass=true_pass,false_pass=false_pass)
            cache_list.append([new_boardhash,depth-1,True,eval])
            #評価値と盤面の対応を保存
            """ if new_board_w==4684619928605818880  and new_board_b==9659097628700567490:
                print(depth-1, True,new_boardhash,eval,"保存") """
            if stop_queue != None:
                if not stop_queue.empty():
                    return None,None
            if other_score:
                other_scores[move_string] = eval
                #transposition_table[boardhash] = eval
            #evals.append(eval)
            if eval < min_eval:
                min_eval = eval
                best_move = move_string
            beta = min(beta, eval)  # αを更新
            if beta <= alpha:  # αβ枝刈り
                #print("β")
                ab_breake = True
                break
        #if min_eval == float("inf"):
            #print("min")
            #print(min_eval)
            #print(evals)
            #print(evals_c)
            #print(legal_list)
        #if not ab_breake:
            #for c in cache_list:
                #zobristhash.save_score(board_hash=c[0],depth=c[1],max_pl=c[2],score=c[3])
        if other_score:
            return min_eval,best_move,other_scores
        return min_eval,best_move



def board_to_bitboard(board, player):
    """ 
    NumPy の 8×8 盤面をビットボードに変換する。
    - board: 8×8 の NumPy 配列（0: 空白, -1: 黒, 1: 白）
    - player: -1 (黒) または 1 (白) を指定
    - 右下 (7,7) を 最下位ビット とする
    """
    bitboard = 0
    for row in range(8):
        for col in range(8):
            if board[row, col] == player:
                index = (7 - row) * 8 + (7 - col)  # 右下を LSB にする
                bitboard |= 1 << index
    return bitboard

def bitboard_to_numpy(bitboard_w, bitboard_b):
    # 64ビット整数をリストに変換して白を1、黒を-1に変換
    board = np.zeros(64, dtype=int)  # 初期化: 0は空白
    board_w = [((bitboard_w >> i) & 1) for i in range(64)]
    board_b = [((bitboard_b >> i) & 1) for i in range(64)]
    
    for i in range(64):
        if board_w[i]:
            board[63-i] = 1  # 白は1
        elif board_b[i]:
            board[63-i] = -1  # 黒は-1

    # リストを8x8のnumpy配列に変換
    board_2d = np.array(board, dtype=int).reshape(8, 8)
    return board_2d

def progress_game(game_nun = 10):
    w_win = 0
    b_win = 0
    rand_win = 0
    minimax_win = 0
    minimax_color = -1
    for _ in range(game_nun):
        b = np.array([#初期の盤面を表す配列
            [0, 0, 0, 0, 0, 0, 0, 0],
            [0, 0, 0, 0, 0, 0, 0, 0],
            [0, 0, 0, 0, 0, 0, 0, 0],
            [0, 0, 0, 1,-1, 0, 0, 0],
            [0, 0, 0,-1, 1, 0, 0, 0],
            [0, 0, 0, 0, 0, 0, 0, 0],
            [0, 0, 0, 0, 0, 0, 0, 0],
            [0, 0, 0, 0, 0, 0, 0, 0]
        ])
        white = board_to_bitboard(b.copy(), 1)&0xFFFFFFFFFFFFFFFF
        black = board_to_bitboard(b.copy(), -1)&0xFFFFFFFFFFFFFFFF
        boards = {1:white,-1:black}
        colors = {1:"white",-1:"black"}
        turn_color = -1
        
        print("対局開始")
        while True:
            turn_legal = get_legal_square(colors[turn_color],boards[1],boards[-1])
            if turn_legal == []:
                turn_color*=-1
                turn_legal = get_legal_square(colors[turn_color],boards[1],boards[-1])
                if turn_legal == []:
                    break
                else:
                    continue
            if minimax_color == turn_color:
                _,act = othello_play_cython2.minimax(boards[turn_color],boards[turn_color*-1],6,alpha=float('-inf'),beta=float('inf'),maximizing_player=True)
            else:
                act = random.choice(turn_legal)
                act = convert_act_bit2str(act)
            white,black = identify_flip_stone(colors[turn_color],boards[1],boards[-1],act,mode=0)
            boards = {1:white,-1:black}
            turn_color*=-1
        w_num=boards[1].bit_count()
        b_num=boards[-1].bit_count()
        if w_num>b_num:
            w_win += 1
            if minimax_color == 1:
                minimax_win += 1
            else:
                rand_win += 1
        elif w_num<b_num:
            b_win += 1
            if minimax_color == -1:
                minimax_win += 1
            else:
                rand_win += 1
        print("終局")
        minimax_color *= -1
    print("白:",w_win,"勝")
    print("黒:",b_win,"勝")
    print("minimax:",minimax_win,"勝")
    print("ランダム:",rand_win,"勝")



if __name__ == "__main__":
    #print(score_flat.shape)
    #print(score_flat)
    #mode1 = input("手入力->0,ランダムで記録->1,棋譜を入力して盤面再現->2,AI(黒)vsRandom->3,AI(白)vsRandom->4を入力:")
    #mode2 = input("自動マウス操作あり->0,なし->1を入力:")
    #mode3 = input("ログを表示する->0,しない->1を入力(手入力を選んだ場合は強制的にで表示):")
    """ r = ""#example
    win_list = []
    num_game = 3
    for game in tqdm(range(num_game)):

        win,record = play_othello("5","1","1",r)
        win_list.append(win)
    b_win = win_list.count("b")
    w_win = win_list.count("w")
    drow = win_list.count("d")
    print("先手(黒):")
    print(str(b_win)+"勝"+str(w_win)+"負"+str(drow)+"分け")
    print("勝率:"+str((b_win/num_game)*100)+"%")
    print("後手(白):")
    print(str(w_win)+"勝"+str(b_win)+"負"+str(drow)+"分け")
    print("勝率:"+str((w_win/num_game)*100)+"%")
    zobristhash.save_table() """
    #import time
    import timeit
    import random
    #print(timeit.timeit("progress_game(4)",globals=globals(),number=1))
    #board = np.zeros((8, 8), dtype=int)

    """ b = np.array([
            [ 1, 1 ,  1,  0,  0,  0,  0, -1],
            [ 1, 1 ,  0,  0,  0,  0,  0,  0],
            [ 1, 0 ,  0,  0,  0,  0,  0,  0],
            [ 1, 0 ,  0,  1,  1,  0,  0,  0],
            [ 1, 0 ,  0,  1,  1,  0,  0,  0],
            [-1, 0 ,  0,  0,  0,  0,  0, -1],
            [ 1, 1 ,  0,  0,  0,  0, -1, -1],
            [ 0, -1 , -1, -1, -1, -1, -1, 0]])
    b = np.array([
            [ 1, 1,  0,  0,  0,  0,  0,  1],
            [ 1, 1,  0,  1,  0,  0,  0,  1],
            [ 0, 0,  0,  0,  1,  0,  0,  1],
            [ 0, 0,  0,  0,  0,  0,  0,  1],
            [ 0, 0,  1,  0,  0,  0,  0,  0],
            [ 1, 0,  0,  1,  0,  0,  0,  1],
            [ 0, 1,  0,  0,  0,  0,  0,  1],
            [ 0, 0,  0,  0,  0,  0,  0,  1]])
    b = np.array([#初期の盤面を表す配列
            [0, 0, 0, 0, 0, 0, 0, 0],
            [0, 0, 0, 0, 0, 0, 0, 0],
            [0, 0, 1,-1, 0, 0, 0, 0],
            [0, 0, 1,-1,-1, 0, 0, 0],
            [0, 0, 1, 1, 1, 0, 0, 0],
            [0, 0, 0, 0, 0, 0, 0, 0],
            [0, 0, 0, 0, 0, 0, 0, 0],
            [0, 0, 0, 0, 0, 0, 0, 0]
        ]) """
    b = np.array([#初期の盤面を表す配列
            [-1, 1, 1, 1, 1, 1, 1, 1],
            [ 1, 1, 1, 1, 1, 1, 1,-1],
            [ 1, 0, 0,-1, 1, 1, 1,-1],
            [ 1, 0, 0,-1,-1, 1, 1,-1],
            [ 1, 0, 1, 1,-1, 1, 1,-1],
            [ 1, 0,-1, 0, 0,-1, 1,-1],
            [ 1, 1, 1, 1, 1, 1, 1,-1],
            [ 0, 1, 1, 1,-1,-1,-1,-1]
        ])
    """ b = np.array([#初期の盤面を表す配列
            [ 0, 0, 0, 0, 0, 0, 0, 0],
            [ 0, 0, 0, 0, 0, 0, 0, 0],
            [ 0, 0, 0, 0, 0, 0, 0, 0],
            [ 0, 0, 0, 1,-1, 0, 0, 0],
            [ 0, 0, 0,-1,-1,-1, 1, 0],
            [ 0, 1, 1, 1, 1, 1, 1, 1],
            [ 0, 0, 0, 0, 1, 0, 0, 0],
            [ 0, 0, 0, 0, 0, 1, 0, 0]
        ])
    b = np.array([#初期の盤面を表す配列
            [ 0, 0, 0, 0, 0, 0, 0, 0],
            [ 0, 0, 1, 0, 0, 1, 0, 0],
            [ 1, 0,-1, 0,-1, 0, 0, 0],
            [ 0,-1,-1,-1,-1, 0, 0, 0],
            [ 1,-1, 0,-1,-1,-1, 1, 0],
            [ 0,-1,-1, 1, 1, 1, 1, 1],
            [ 1, 0, 1, 0, 1, 0, 0, 0],
            [ 0, 0, 0, 0, 0, 1, 0, 0]
        ]) """
    """ b = np.array([
            [ 1, -1, -1, -1, -1, -1,  1,  1],
            [-1,  0,  0,  0,  1,  1,  1,  1],
            [-1,  0,  0, -1,  1,  0,  1,  1],
            [-1,  0,  0,  1,  1,  1,  1, -1],
            [-1, -1,  0,  0,  0,  0,  0, -1],
            [-1, -1,  0,  0,  0, -1,  1, -1],
            [-1, -1,  0,  0,  0,  0, -1, -1],
            [ 1, -1, -1, -1, -1, -1, -1,  1]]) """
    b = np.array([#初期の盤面を表す配列
        [0, 0, 0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0, 0, 0],
        [0, 0, 0, 1,-1, 0, 0, 0],
        [0, 0, 0,-1, 1, 0, 0, 0],
        [0, 0, 0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0, 0, 0]
    ])
    white = board_to_bitboard(b.copy(), 1)&0xFFFFFFFFFFFFFFFF
    black = board_to_bitboard(b.copy(), -1)&0xFFFFFFFFFFFFFFFF
    #white2=4684619928605818880
    #black2=9659097628700567490
    print(white)
    print(black)
    #print(get_legal_square("white",white,black))
    #t1 = time.time()
    #white = 9659388999281902018
    #black = 4612266565940036096
    #print(bin(white))
    #print(bin(black))
    #def a():
    #    #evaluate_board(white,black)
    #    minimax(black,white,5,alpha=float('-inf'),beta=float('inf'),maximizing_player=True)
    #print(timeit.timeit("a()",globals=globals(),number=10))
    #
    #e = get_legal_square("white",white,black)
    #print(evaluate_board(b))
    #print(get_legal_square("black",white,black))
    #w,b = identify_flip_stone("black",white,black,"a8",mode=0)
    #print(w,b)
    #print(bitboard_to_numpy(w,b))
    #w_c,b_c  = get_confirmed_stones(white,black)
    #w_c2,b_c2 = w_c.bit_count(),b_c.bit_count()
    #print(bitboard_to_numpy(w_c,0))
    #print(bitboard_to_numpy(w_c,b_c))
    #print(w_c.bit_count(),b_c.bit_count())
    #print(minimax(black,white,2,alpha=float('-inf'),beta=float('inf'),maximizing_player=True))
    #print(minimax(white2,black2,1,alpha=float('-inf'),beta=float('inf'),maximizing_player=True))
    #e = evaluate_board(white,black)
    #print(e)
    #print(e)
    #print(minimax(black,white,3,alpha=float('-inf'),beta=float('inf'),maximizing_player=True))
    #t2 = time.time()
    #print(e)
    #print(t2-t1)
    #print(bin(w_c))
    #print(w_c2,b_c2)
    
    """ for i in range(len(m_c_list)):
        print(bitboard_to_numpy(m_c_list[i],0))
        print(bitboard_to_numpy(m_e_list[i],0)) """

    """ f = 0xF0F0F0F00F0F0F0F
    f2 =0x00000000FFFFFFFF
    f3 = 0xF0F0F0F0F0F0F0F0
    f4 = 0x0000333300003333
    f5 = 0xCCCC0000CCCC0000
    f6 = 0x3333333333333333
    f7 = 0xCCCCCCCCCCCCCCCC
    f8 = 0x0055005500550055
    f9 = 0xAA00AA00AA00AA00
    f10 =0x5555555555555555 
    f11 =0xAAAAAAAAAAAAAAAA 

    m1 = 0x0080808080808000
    m1 = 0x8000000000000080 """
    #t2m = white
    #f = (t2&mask4_r)<<2|(t2&mask4_l)>>2#2*2左右ミラー
    #f = ((t2m&mask1)<<36|(t2m&mask1)>>36)|(t2m&(~mask1&safety_maask))
    #board_combined = bitboard_to_numpy((white&f)>>36, 0)
    #w2 = rotate90(white)
    #board_combined = bitboard_to_numpy(m1, 0)
    #print(board_combined)
    #print(minimax(black,white,1,alpha=float('-inf'),beta=float('inf'),maximizing_player=True))


    #print(win,record)
    """ board = np.array([
        [-1,-1,-1,-1,-1,-1,-1,-1],
        [-1,-1,-1,-1,-1,-1, 1,-1],
        [-1,-1,-1,-1,-1,-1,-1,-1],
        [-1,-1,-1,-1,-1,-1,-1,-1],
        [-1,-1,-1,-1,-1,-1,-1,-1],
        [-1,-1,-1,-1,-1,-1,-1,-1],
        [-1,-1,-1,-1,-1,-1,-1,-1],
        [-1,-1,-1,-1,-1,-1,-1,-1]
    ])
    b_c,w_c = get_confirmed_stones(board)
    print("b:",b_c,"\nw:",w_c) """

    #m = 0xFF818181818181FF
    #m = 0x42C300000000C342
    #print(bitboard_to_numpy(m,0))
    

    #legal_squares = get_legal_square("white",first_board)
    #print(legal_squares)
    #全部空の配列を召喚
    #empty_board = np.zeros((8,8),np.int64)

    #print(empty_board)
    #デバッグ用
    #for i in legal_squares:
    #    empty_board[i] = 1
    #print(empty_board)

    #result = identify_flip_stone("black",first_board,"d3")
    #print(result)
    aa = 0b101
    bb = 0b111
    print(aa.bit_count())
    print(aa.bit_length())