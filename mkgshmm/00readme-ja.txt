    mkgshmm

MKGSHMM(1)                                                          MKGSHMM(1)



名前
           mkgshmm
          - モノフォンHMMを GMS 用に変換する

概要
       mkgshmm {monophone_hmmdefs}
                 >
                  {outputfile}

DESCRIPTION
       mkgshmm はHTK形式のmonophone HMMを Julius の Gaussian Mixture Selection
       (GMS) 用に変換するperlスクリプトです．

       GMSはJulius-3.2からサポートされている音響尤度計算の高速化手法です． フ
       レームごとに monophone の状態尤度に基づいてtriphoneやPTMの状態を予 備選
       択することで，音響尤度計算が高速化されます．

EXAMPLES
       まずターゲットとするtriphoneやPTMに対して，同じコーパスで学習した
       monophone モデルを用意します．

       次にそのmonophoneモデルを mkgshmm を用いて GMS 用に変換します．
       これを Julius で "-gshmm" で指定します．
       GMS用モデルはtriphoneやPTMと同一のコーパスから作成する必要がある点に注
       意してください．gshmm がミスマッチだと選択誤りが生じ，性能が劣化しま
       す．

SEE ALSO
        julius ( 1 )

COPYRIGHT
       Copyright (c) 1991-2013 京都大学 河原研究室

       Copyright (c) 1997-2000 情報処理振興事業協会(IPA)

       Copyright (c) 2000-2005 奈良先端科学技術大学院大学 鹿野研究室

       Copyright (c) 2005-2013 名古屋工業大学 Julius開発チーム

LICENSE
       Julius の使用許諾に準じます．



                                  19/12/2013                        MKGSHMM(1)
