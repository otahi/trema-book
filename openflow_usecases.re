= OpenFlow のユースケース

//lead{
OpenFlowは一見何にでも使えそうですが、本当にそうでしょうか？OpenFlowがぴったりとはまる使いみちを考えてみましょう。
//}

== OpenFlow はアカデミア出身

OpenFlowスイッチには「コントローラさえ書けばどんなネットワーク機器にも化けられる」という特長があります。乱暴に言うと、スイッチのような単純な機器はもちろん、ルータやロードバランサ、ファイアウォールやNATなど複雑な機器も、コントローラの実装をがんばれば実現できてしまいます。もちろん、専用機と同じ機能をすべて実装するのは大変ですし、機能の一部をソフトウェアとして実装することになるので実装が悪いと性能は落ちます。しかし、ソフトウェア次第で何でもできることに変わりはありません。

この何でもできるという特長は、もともとは大学や研究所などアカデミアからのニーズによって生まれたものでした。今までのスイッチやルータにとらわれない、まったく新しいインターネットを研究したい。でもすでにあるスイッチやルータのファームウェアを改造するのは大変。そうかと言って一からハードウェアは作りたくないし…大規模な仮想ネットワーク上で実験してもいいけど、それだと実際のインターネット環境とあまりにも違いすぎる。こうしたジレンマを解消するために考え出されたのがOpenFlowだったわけです。

== なぜOpenFlowが注目されているのか？

では、インターネットの研究用だったOpenFlowが巨大データセンターに代表される産業界でも注目されるようになったのはなぜでしょうか？

データセンターで最も重視されるのはスループット(単位時間あたりの処理能力)とコストです。Googleなどの巨大データセンターでは、世界中から届くたくさんのリクエストを短時間でさばくために(つまり、スループットを上げるために)物理的にたくさんのスイッチやサーバをそろえる必要があります。もし1台1台の価格が高いと、ハードウェア代だけでコストが膨大になってしまいます。そのため、秋葉原でも買えるコモディティ(普及品)から構成される安いハードウェアを使います。

データセンターの信頼性はハードウェアではなくソフトウェアでカバーします。巨大データセンターではとにかく大量のサーバやスイッチを使うため、個々のハードウェアの信頼性はそれほど重視しません。数十万台のサーバがある環境では、そもそもすべてが故障なしに動き続けることはまったく期待できないからです。そのかわり、上位のソフトウェア層で信頼性を担保します。死活監視や冗長化、障害からの自動復旧など信頼性にかかわる部分はすべてミドルウェアとして実装するのです。

OpenFlowはこのデータセンターのモデルにうまくマッチしています。OpenFlowは制御を行うソフトウェアであるコントローラと、その命令に従うだけのハードウェアであるOpenFlowスイッチにきれいに分離できます。これは、データセンターのモデル--全体を制御し信頼性を確保するミドルウェアと、大量のコモディティハードウェア--と似ています。ネットワーク制御はソフトウェアとして実装するので、死活監視や冗長化、自動復旧などで信頼性を担保するミドルウェアとの連携が容易です。ネットワークとアプリケーションがそれぞれ独立して信頼性を確保するよりも、無駄が少なく、かつ高い信頼性を実現できます。

そして何よりも、データセンターはOpenFlowのような新しい技術を採用しやすいプラットフォームです。データセンターの増設は、サーバ単位ではなく、フロア単位もしくは建物単位と、かなり大きな単位で行います。そのため、既存部分との相互接続性を気にする必要は少なく、独自の新技術を導入しやすいのです。

このように「柔軟性があり既存ミドルウェアとの連携が容易」というOpenFlowの設計が、たまたまデータセンターにマッチした、これがOpenFlowに注目が集まる理由です。

== ユースケースあれこれ

ここでは、スイッチやルータなど基本的なネットワーク部品をOpenFlowで実装する方法をみていきます。その前に、OpenFlowでできることをおさらいしておきましょう。

「OpenFlowでできることは何か？」という質問を言いかえると、「フローでできることは何か？」になります。OpenFlowスイッチがフローテーブルに従って次々とパケットを転送する様子は、筆者が小学生のころよく楽しんだ水道管ゲームを思い出させます。これは水道管のバルブから蛇口まで、さまざまな形の水道管のカードをならべて水を届けるというゲームです。フローでできることはこれによく似ています。できることは基本的には次の4つです。

 1. パケットを転送する
 2. 流量を調べる
 3. 書き換える
 4. 分岐する

これがすべてです。パケットを指定したスイッチポートから出力することで転送する。転送されたパケットの量を調べる。パケットのヘッダを書き換える。パケットを複製して複数のスイッチポートから出力する。これらを自由に組み合わせて、さまざまなタイプのネットワークを作れます。

それではいよいよ実際のユースケースを見ていきましょう。上の4種類のカードをどのように組み合わせて実現しているかというところに注目してください。

=== スイッチ

「パケットを転送する」を使うと、最もシンプルなスイッチをOpenFlowで実現できます(@<img>{switch})。スイッチは届いたパケットの宛先のMACアドレスを見て、そのMACアドレスを持つホストがつながるポートへとパケットを転送します。

//image[switch][OpenFlowでスイッチを実現する][width=12cm]

これに「流量を調べる」を組み合わせると、スイッチにトラフィック集計機能を付け加えられます。フローエントリごとにどれだけのパケットを転送したかを集計することで、コントローラでネットワーク全体のトラフィックを集計できます。

//image[traffic_switch][OpenFlowでトラフィック集計機能つきスイッチを実現する][width=12cm]

なお、スイッチのOpenFlowでの詳しい実装方法は@<chap>{learning_switch}で、またトラフィック集計機能は@<chap>{traffic_monitor}で説明します。

=== ルータ

「パケットを転送する」に「書き換える」を組み合わせると、ルータをOpenFlowで実現できます(@<img>{router})。ルータは異なる2つのネットワークの間で動作し、ネットワーク間のパケットのやりとりに必要な転送と書き換え処理を行います。パケットがルータを通るとき、ルータはパケットの宛先と送信元のMACアドレスを書き換えて転送します。

//image[router][OpenFlowでルータを実現する][width=12cm]

なお、ルータのOpenFlowでの詳しい実装方法は@<chap>{router_part1}および@<chap>{router_part2}で説明します。

=== ロードバランサ

ルータに「流量を調べる」を追加して改造すると、いわゆるロードバランサをOpenFlowで実現できます(@<img>{load_balancer})。ロードバランサとは、Webサーバなどアクセスが集中しやすいサーバの負荷を下げるため、クライアントからのアクセスを複数台のバックエンドサーバに振り分けるネットワーク装置またはソフトウェアです。

//image[load_balancer][OpenFlowでロードバランサを実現する][width=12cm]

ロードバランサは次のように動作します。

 1. クライアントからのリクエストが届くと、担当するバックエンドサーバを決める
 2. リクエストパケットをバックエンドサーバに届けるために書き換える(ルータと同じ)
 3. バックエンドサーバがつながるスイッチポートに書き換えたパケットを出力する

//noindent
アクセスの状況によっては、バックエンドサーバの間で負荷に偏りが出ることがあります。「流量を調べる」を使うと、空いているバックエンドへ優先して振り分けられます。

必要なバックエンドサーバの数は、時間帯によって異なります。たとえば、アクセスが減る夜中になればバックエンドサーバ数を減らせます。逆に昼食後などのアクセスが増えやすい時間帯では、バックエンドサーバを増やす必要があります。

もしバックエンドサーバの数を調節できるAPIがあれば、ロードバランシングとバックエンドサーバ数の調節を連携できます(@<img>{advanced_load_balancer})。「流量を調べる」でトラフィックを調べ、これに応じてバックエンドサーバ数をAPI経由で自動調節できるからです。

//image[advanced_load_balancer][トラフィックに応じてバックエンドサーバの数を調節する高度なロードバランサの例][width=12cm]

このように、コントローラはバックエンドサーバのような既存ミドルウェアと連携することでネットワークに機能を追加できます。コントローラは主要なプログラミング言語で実装できるので (@<chap>{openflow_frameworks}で紹介)、さまざまな既存ミドルウェアのAPIを通じて楽に連携できます。

さて、ここまでで基本的なネットワーク機器をOpenFlowで実装する例を紹介してきました。ここからはより細かく、ネットワーク経路の制御をOpenFlowで行うパターンをいくつか見ていきます。

=== 帯域をめいっぱい使う

「パケットを転送する」を使うと、複数の経路を使ってVMイメージなどの大きなデータを効率的に転送できます。転送元から転送先へ複数のコネクションを張り、それぞれ専用の経路をめいっぱい使って転送できるのです(@<img>{maximize_bandwidth})。

//image[maximize_bandwidth][複数経路を使って帯域をかせぐ][width=12cm]

この「帯域をめいっぱい使う」の具体的な例は、@<chap>{google}で紹介します。

=== パケットを複製する

「分岐する」を使えば、サーバが送信したパケットを、スイッチがコピーして、複数のクライアントに届けることができます(@<img>{multicast})。クライアントの数が増えても、スイッチがパケットを複製してくれるので、サーバから送るパケットの数を増やす必要はありません。「分岐する」を使わなければ、サーバはクライアントそれぞれに対して、同じパケットを送る必要があります。これと比べ、「分岐する」を使えば、ネットワークの帯域を節約することができます。

//image[multicast][パケットを複製し、複数のクライアントに届ける][width=12cm]

また、「分岐する」で冗長な経路を作り、それぞれの経路に複製したパケットを流せば、ネットワークに障害が起こった場合でもパケットをロスせずに復旧できます。クライアントとサーバが通信する状況を考えてください(@<img>{fail_over})。一番左のスイッチは、複製したパケットを、2 つの経路それぞれに送ります。受け取る側は、冗長化された経路のうちどちらか一方だけから受け取るようにしておきます。このようにしておけば、もしどちらかの経路に障害が起こっても、もう一方の経路からパケットを受け取れるので、パケットを一つも落とさずに通信を続けられます。

//image[fail_over][OpenFlow で冗長な経路を作る][width=12cm]

ここで重要なのは、コントローラは全体の状況を見て自由自在に経路を決めることができるということです。従来のネットワークでは、各スイッチが個別に最適となるような経路を決めるため最短路など特定の経路しか取れませんでした。逆にOpenFlowでは、コントローラが全体の道路と交通状況を逐一把握しており、好きなポイントで自由に経路を切り替えられます。

=== 自由にネットワーク構成を作る・変更する

OpenFlowを使うと、ネットワーク構成を物理的な構成にしばられることなく自由に構成できます。例えば、「パケットを転送する」を使うとホストの所属するネットワークを簡単に切り替えられます。@<img>{patch_panel}のようにスイッチにホスト2つとネットワーク2つが接続していると考えてください。コントローラはこれに「パケットを転送する」フローエントリを打ち込むことで、ホストとネットワークの接続を自由に切り替えられます。たとえばホストAがつながるポートとネットワークBのつながるポートでパケットを相互に転送すれば、ホストAはネットワークBに属するようになります。

//image[patch_panel][各ホストの所属するネットワークを切り替える][width=12cm]

こうした機能を持つハードウェアをパッチパネルと呼び、@<chap>{patch_panel}で詳しく説明します。より高度な例については、@<chap>{sliceable_switch}および@<chap>{datacenter_wakame}で紹介します。

== まとめ

OpenFlowがうまく行く具体的なユースケースを見てきました。OpenFlowではプログラミング次第で好きな機能を追加できますが、専用のネットワーク機器と同じフル機能を実装するのは現実的ではありません。そのかわりデータセンターに代表される、ソフトウェアでハードウェア全体を制御する世界とうまくマッチします。ネットワーク管理の自動化や最適化、そしてスループットの向上に必要な機能をOpenFlowで実装する、それがOpenFlowの最も威力を発揮する使い方です。

次章ではいよいよOpenFlowの仕様を少し詳しく紹介します。

== 参考文献

: Googleクラウドの核心(ルイス・アンドレ・バロッソら著、日経BP社)
  Googleの巨大データセンターはウェアハウス・スケール・コンピュータ(WSC, 倉庫規模のコンピュータ)とも呼ばれ、何万ものサーバが詰め込まれた巨大倉庫を一台のコンピュータとしてソフトウェアで制御します。実際のデータセンター運用者でしか知り得ない豊富なデータを元に、WSC全体のアーキテクチャとその中で使われるミドルウェア等の設計を議論しています。
