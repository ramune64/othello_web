import pandas as pd
import scipy.stats as stats

import seaborn as sns
import matplotlib.pyplot as plt

# CSV読み込み
df = pd.read_csv("csvs/diff_stone.csv")

# プレイヤーごとに色を変えて描画
""" plt.figure(figsize=(12, 6))
for i in range(9):
    if i==7:
        #sns.kdeplot(df[str(i)], label=f"Origin",fill=True)
        #sns.histplot(df[str(i)], bins=30, kde=False,label="Origin")
        pass
    elif i!=7:
        sns.kdeplot(df[str(i)]-df[str(7)], label=f"Player {i}")
        #sns.histplot(df[str(i)]-df[str(7)], bins=30, kde=False,label=f"Player {i}") """

""" plt.title("Stone Difference Distribution (by Player)")
plt.xlabel("Stone Difference")
plt.ylabel("Density")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show() """

#df = pd.read_csv("csvs/diff_stone.csv")
""" for i in range(9):
    #stat, p = stats.shapiro(df[str(i)])  # "0" はプレイヤー番号
    
    stat, p = stats.kstest(df[str(i)]-df[str(7)], "norm")

    print(f"Shapiro-Wilk Test stat={stat}, p={p}") """

import matplotlib.pyplot as plt
import scipy.stats as stats

""" stats.probplot(df["0"]-df[str(7)], dist="norm", plot=plt)
plt.title("Q-Q plot (Player 0)")
plt.grid(True)
plt.show() """

import  math
import numpy as np

for i in range(9):
    #print(sum(df[str(i)])/len(df[str(i)]))
    if i==7:continue
    
    player = df[str(i)]
    origin = df["7"]
    
    # 対応のあるt検定（同じ対戦条件下の比較）
    t_stat, p_val = stats.ttest_ind(player, origin,equal_var=False)
    print(f"Player {i} vs Origin: t={t_stat}, p={p_val}")

    """ # Player i vs Origin（例: i = 0）
    mean_diff = np.mean(player) - np.mean(df["7"])
    pooled_std = np.sqrt((np.var(player, ddof=1) + np.var(df["7"], ddof=1)) / 2)
    cohen_d = mean_diff / pooled_std
    print(cohen_d) """