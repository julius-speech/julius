    mkdfa.pl

MKDFA.PL(1)                                                        MKDFA.PL(1)



名前
           mkdfa.pl
          - Julius 形式の文法をオートマトンに変換するコンパイラ

概要
       mkdfa.pl [options...] {prefix}

DESCRIPTION
       mkdfa.pl は Julius の文法コンパイラです．記述された文法ファイル
       (.grammar) と語彙ファイル (.voca) から，Julius用の有限状態オートマトン
       ファイル (.dfa) および認識辞書 (.dict) を生成します．カテゴリ名と生成
       後の各ファイルで用いられるカテゴリ ID 番号との対応が .term ファイルと
       して出力されます．

       各ファイル形式の詳細については，別途ドキュメントをご覧下さい．

       prefix は，.grammar ファイルおよび .vocaファイルの プレフィックスを引数
       として与えます．prefix.grammarと prefix.vocaからprefix.dfa，
       prefix.dictおよび prefix.termが生成されます．

       バージョン 3.5.3 以降の Julius に付属の mkdfa.pl は， dfa_minimize を内
       部で自動的に呼び出すので， 出力される .dfa は常に最小化されています．

OPTIONS
        -n
           辞書を出力しない．.voca 無しで .grammar のみを .dfa に変換する こと
           ができる．

ENVIRONMENT VARIABLES
        TMP または TEMP
           変換中に一時ファイルを置くディレクトリを指定する． 指定が無い場合，
           /tmp, /var/tmp, /WINDOWS/Temp, /WINNT/Temp の順で最初に見つかった場
           所が使用される．

EXAMPLES
       文法ファイル foo.grammar, foo.vocaに 対して以下を実行することで
       foo.dfaと foo.vocaおよびfoo.termが出力される．

SEE ALSO
        julius ( 1 ) ,
        generate ( 1 ) ,
        nextword ( 1 ) ,
        accept_check ( 1 ) ,
        dfa_minimize ( 1 )

DIAGNOSTICS
       mkdfa.pl は内部で mkfa および dfa_minimize を呼び出します．実行時，これ
       らの実行ファ イルが，この mkdfa.pl と同じディレクトリに置いてある必要が
       あります． これらはJulius に同梱されています．

COPYRIGHT
       Copyright (c) 1991-2013 京都大学 河原研究室

       Copyright (c) 1997-2000 情報処理振興事業協会(IPA)

       Copyright (c) 2000-2005 奈良先端科学技術大学院大学 鹿野研究室

       Copyright (c) 2005-2013 名古屋工業大学 Julius開発チーム

LICENSE
       Julius の使用許諾に準じます．



                                  19/12/2013                       MKDFA.PL(1)
