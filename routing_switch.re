= ルーティングスイッチ

多数のスイッチ、ルータから構成される大規模なネットワークの構成図を見ると、
ネットワークが好きな人は心が踊ります。
``どんなプロトコルで動いているのか?'' を知りたくなったり、
``いつか自分でも動かしてみたい!'' と思ったりするものです。
ここまでの章では一台の OpenFlow スイッチを制御する方法について学んできましたが、
もちろん、OpenFlow は、複数台のスイッチからなる大規模なネットワークでも使えます。

では、どのように大規模ネットワークを制御すればよいでしょうか？
スイッチを自由に操ることのできる OpenFlow を使えば、さまざまな制御方法を試せそうです。
本書で Trema の使い方をマスターすれば、オリジナルの制御方法を作り出せるかもしれません。

そうは言ってもオリジナルの方法を一から考えだすのは、少々骨が折れそうなので、
何かを参考にしたいものです。
Trema Apps には、複数台の OpenFlow スイッチをまとめて制御する
ルーティングスイッチというアプリケーションが用意されています。
本章では、大規模ネットワーク制御の一例として、ルーティングスイッチが
どのように動作するかについて紹介を行います。

#@warn(Trema Apps の記述について全体と整合を取る。ここが初出ならもっと詳しく) 

== 複数のスイッチを扱う

ルーティングスイッチは、@<chap>{learning_switch} で扱ったラーニングスイッチと同様に
普通の L2 スイッチとして動作するアプリケーションです。
ラーニングスイッチは、1 台の OpenFlow スイッチが 1 台の L2 スイッチとして
動作するよう制御するコントローラでした。
それに対してルーティングスイッチを使えば、複数台の OpenFlow スイッチが連動して、
一つの大きな L2 スイッチとして動作します (@<img>{routing_switch})。

//image[routing_switch][ルーティングスイッチ]

=== ルーティングスイッチの動作

@<img>{flow_mod} のネットワーク中で、
ホスト 1 からホスト 4 へとパケットを届けるためには
どうすればよいでしょうか？
ホスト 1 が接続するスイッチ 1 から、スイッチ 5、スイッチ 6 の順に
パケットが送られ、ホスト 4 へと届くようにフローを設定すればよさそうです。

//image[flow_mod][フローの設定]

//image[behavior][ルーティングスイッチの動作]

それでは、コントローラの動作を、@<img>{behavior} を使って、
順に見ていきましょう。

 1. ルーティングスイッチは、 スイッチが受信したパケットを Packet In メッセージで受け取ります。@<img>{flow_mod} の例では、ホスト 1 からホスト 4 へのパケットが、スイッチ 1 から送られてきます。
 2. 次に fdb を検索し、宛先であるホストが接続されているスイッチ、ポートを検索します。
 3. Packet In を送ってきたスイッチから出口となるスイッチまでの最短パスを計算します。この場合、スイッチ 1 からスイッチ 6 の最短パスとして、スイッチ 5 を経由するパスが得られます。
 4. 最短パスに沿ってパケットが転送されるよう、各スイッチそれぞれに Flow Mod メッセージを送り、フローを設定します。
 5. その後、Packet In でコントローラに送られたパケットを 2 で決定したスイッチのポートから出力するよう、Packet Out メッセージを送ります。

基本的な動作はラーニングスイッチと同じですが、
パケットを受信したスイッチと違うスイッチまでパケットを届けなければ
ならない点が異なっています。
そのためには、入口となるスイッチから出口となるスイッチまでの
最短パスを見つける必要があります。

#@warn(ラーニングスイッチの章と整合を取るように。比較して理解しやすいように)

=== 最短パスを計算する

それでは、スイッチ 1 からスイッチ 6 までのパスは、どのように計算すればよいでしょうか？
ルーティングスイッチでは、ダイクストラ法というアルゴリズムを用いて、
二つのスイッチ間の最短パスを求めています。
ダイクストラ法は、ネットワークの世界では非常にポピュラーなアルゴリズムで、
OSPF や IS-IS 等の L3 の経路制御プロトコルで用いられています。
ここでは、ダイクストラ法を用いて最短パスを見つける方法について、
簡単な説明を@<img>{dijkstra}を用いて行います。

//image[dijkstra][最短パスの計算]

まず始点となるスイッチ 1 に着目します (@<img>{dijkstra} (a))。

初めに、スイッチ 1 から 1 ホップで行けるスイッチを見つけ出します。
これはスイッチ 1 に接続するリンクの先に繋がっているスイッチを、
すべてピックアップすれば見つけ出せます。
その結果、スイッチ 2 とスイッチ 5 が見つけ出せます (@<img>{dijkstra} (b))。

同じようにして、今度はスイッチ 1 から 2 ホップで行けるスイッチを見つけ出します。
これは、今見つけたスイッチ 2 とスイッチ 5 から 1 ホップで行けるスイッチから
見つけ出せばよさそうです。このようにして、スイッチ 3, 4, 6 が見つかります 
(@<img>{dijkstra} (c))。

スイッチ 6 はスイッチ 5 の先で見つかりましたので、
最終的にスイッチ 1 → スイッチ 5 → スイッチ 6 というパスが最短パスであることが
わかります。

=== トポロジーを検出する

ダイクストラ法には、``リンクの先に繋がっているスイッチを調べる'' 
というステップがありました。
@<img>{routing_switch} のように、ネットワーク全体のトポロジーを
コントローラが知っている必要があります。
ここでは、スイッチ同士がどのように繋がっているかを調べる方法について
見て行きましょう。

トポロジーの検出には、Link Layer Discovery Protocol（LLDP）を用います
（@<img>{lldp}）。
コントローラから Packet Out により送信された LLDP パケットは、
LLDP が到達した隣のスイッチから Packet In でコントローラへと戻ります。

//image[lldp][トポロジーの検出]

コントローラは、LLDP パケットを作る際に、Packet Out を送るスイッチの 
Datapath ID と、LLDP パケットを出力するポートの番号を埋め込みます。
LLDP パケットが Packet In でコントローラへと戻る際には、
受信ポートの番号が合わせて通知されます。
コントローラは、これらの情報を総合することで、二つのスイッチ同士が、
どのポート経由で接続するかを知ることができます。
これを繰り返すことで、ネットワーク中のすべてのスイッチ間の
接続関係を知ることができます。

この方法は、仕様で特に規定されているわけではありませんが、
OpenFlow でよく用いられています。Packet Out と
Packet In を用いた ``OpenFlow ならでは'' の方法です。

== 実行してみよう

ルーティングスイッチは、Trema Apps の一部として、
Github 上でソースコードを公開しています。
このソースコードを使って、ルーティングスイッチの動作を見ていきましょう。

=== 準備

Trema Apps のソースコードは、@<tt>{https://github.com/trema/apps/} に
あります。
まずは、@<tt>{git} を使って、ソースコードを取得しましょう。
以下のように @<tt>{trema} ディレクトリと同じ階層になるよう
取得してください。

//cmd{
% ls -F
trema/
% git clone https://github.com/trema/apps.git
...
% ls -F
apps/	trema/
//}

@<chap>{trema_architecture} で紹介したように、
Trema Apps にはさまざまなアプリケーションが含まれています。
そのうち、今回使用するのは @<tt>{topology} と @<tt>{routing_switch} です。
@<tt>{topology} には、トポロジー検出を担当するモジュール 
@<tt>{topology_discovery} と検出したトポロジーを管理するモジュール @<tt>{topology} が含まれています。
また @<tt>{routing_switch} には、ルーティングスイッチの本体が含まれています。
この二つを順に @<tt>{make} してください。

//cmd{
% (cd apps/topology/; make)
% (cd apps/routing_switch; make)
//}

=== ルーティングスイッチを動かす

それでは、ルーティングスイッチを動かしてみましょう。
今回は Trema のネットワークエミュレータ機能を用いて、
@<img>{fullmesh} のようなネットワークを作ってみます。

//image[fullmesh][ネットワーク構成]

このネットワーク構成を作るためには、@<list>{conf} の
ように記述を行う必要があります。

//list[conf][@<tt>{routing_switch_fullmesh.conf}]{
vswitch {
  datapath_id "0xe0"
}

vswitch {
  datapath_id "0xe1"
}

vswitch {
  datapath_id "0xe2"
}

vswitch {
  datapath_id "0xe3"
}

vhost ("host1") {
  ip "192.168.0.1"
  netmask "255.255.0.0"
  mac "00:00:00:01:00:01"
}

vhost ("host2") {
  ip "192.168.0.2"
  netmask "255.255.0.0"
  mac "00:00:00:01:00:02"
}

vhost ("host3") {
  ip "192.168.0.3"
  netmask "255.255.0.0"
  mac "00:00:00:01:00:03"
}

vhost ("host4") {
  ip "192.168.0.4"
  netmask "255.255.0.0"
  mac "00:00:00:01:00:04"
}

link "0xe0", "host1"
link "0xe1", "host2"
link "0xe2", "host3"
link "0xe3", "host4"
link "0xe0", "0xe1"
link "0xe0", "0xe2"
link "0xe0", "0xe3"
link "0xe1", "0xe2"
link "0xe1", "0xe3"
link "0xe2", "0xe3"

run {
  path "../apps/topology/topology"
}

run {
  path "../apps/topology/topology_discovery"
}

run {
  path "../apps/routing_switch/routing_switch"
}

event :port_status => "topology", :packet_in => "filter", :state_notify => "topology"
filter :lldp => "topology_discovery", :packet_in => "routing_switch"
//}

このファイルは、@<tt>{routing_switch_fullmesh.conf} として、
ソースコード一式の中に用意されています。
今回はこのファイルを使って、以下のように起動してください。

//cmd{
% cd ./trema
% ./trema run -c ../apps/routing_switch/routing_switch_fullmesh.conf -d
//}

=== 見つけたリンクを表示する

@<tt>{topology} ディレクトリには、検出したトポロジーを表示するコマンドが
用意されていますので、使ってみましょう。
以下のように実行してください。

//cmd{
% TREMA_HOME=. ../apps/topology/show_topology -D
vswitch {
  datapath_id "0xe0"
}

vswitch {
  datapath_id "0xe2"
}

vswitch {
  datapath_id "0xe3"
}

vswitch {
  datapath_id "0xe1"
}

link "0xe3", "0xe2"
link "0xe1", "0xe0"
link "0xe3", "0xe0"
link "0xe2", "0xe1"
link "0xe2", "0xe0"
link "0xe3", "0xe1"
//}

ルーティングスイッチの起動時に指定した設定ファイル (@<list>{conf}) や
ネットワーク構成 (@<img>{fullmesh}) と
比較してみましょう。@<tt>{topology_discovery} モジュールが検出できるのは、
スイッチ間のリンクのみです。
仮想ホストとスイッチ間のリンクは検出できないため、@<tt>{show_topology} の
検出結果には表示されないことに注意しましょう。

=== パケットを送り、フローが設定されているかを確認する
   
次に、仮想ホストからパケットを送り、フローが設定されることを確認しましょう。

//cmd{
% ./trema send_packets --source host1 --dest host2
% ./trema send_packets --source host2 --dest host1
//}

ルーティングスイッチ起動直後は、まだ MAC アドレスの学習を行なっていないので、
host1 から host2 へとパケットを送っただけではフローは設定されません。
この段階で host1 の MAC アドレスを学習したので、
host2 から host1 へと送った段階でフローが設定されます。

それでは、どのようなフローが設定されたかを見てみます。
設定されているフローの確認は、
@<tt>{trema dump_flows [表示したいスイッチの Datapath ID]} でできます。
@<tt>{0xe0} から @<tt>{0xe1} まで順に表示してみましょう。

//cmd{
% ./trema dump_flows 0xe0
NXST_FLOW reply (xid=0x4):
 cookie=0x3, duration=41s, table=0, n_packets=0, n_bytes=0, idle_timeout=62, \
 ...	     		   	    		 	    		     \
 dl_src=00:00:00:01:00:02,dl_dst=00:00:00:01:00:01,nw_src=192.168.0.2,	     \
 nw_dst=192.168.0.1,nw_tos=0,tp_src=1,tp_dst=1 actions=output:3
% ./trema dump_flows 0xe1
NXST_FLOW reply (xid=0x4):
 cookie=0x3, duration=42s, table=0, n_packets=0, n_bytes=0, idle_timeout=61, \
 ...	     		   	    		 	    		     \
 dl_src=00:00:00:01:00:02,dl_dst=00:00:00:01:00:01,nw_src=192.168.0.2,	     \
 nw_dst=192.168.0.1,nw_tos=0,tp_src=1,tp_dst=1 actions=output:3
% ./trema dump_flows 0xe2
NXST_FLOW reply (xid=0x4):
% ./trema dump_flows 0xe3
NXST_FLOW reply (xid=0x4):
//}

@<tt>{0xe0} と @<tt>{0xe1} のスイッチそれぞれに、@<tt>{dl_src} が host2 の
MAC アドレス、@<tt>{dl_dst} が host1 の MAC アドレスがマッチングルールの
フローが設定されていることが分かります。
一方で @<tt>{0xe2} と @<tt>{0xe3} のスイッチには、フローがありません。
@<img>{fullmesh} をもう一度見てください。host2 から host1 への最短パスは
@<tt>{0xe1} → @<tt>{0xe0} なので、この二つのスイッチにきちんとフローが
設定されています。

== 利点・欠点

 * スパニングツリーがいらなくなる点
 * 集中制御なので、いろいろなアルゴリズムを選べる点
 * パーフローでの処理なので、フィルタが容易に実装できる点

== まとめ/参考文献

本章で学んだことは、次の 3 つです。

 * 複数のスイッチからなる大規模ネットワークを扱うことができる、
   ルーティングスイッチがどのように動作するかを見てみました。
 * トポロジーを検出する仕組みを見てみました。またエミュレータ機能上の
   ネットワークで実際に、トポロジーが検出できることを確認しました。
 * 最短パスを計算する方法について学びました。エミュレータ上で通信を
   行った結果、最短パス上にフローが設定される様子を見てみました。

: マスタリング TCP/IP 応用編
  L3 の経路制御プロトコルについて詳しく説明されています。本章で扱ったダイクストラ法を
  用いた経路制御プロトコルである OSPF についても説明がされているので、
  ルーティングスイッチとの違いを比べてみるのも面白いかもしれません。

: 最短経路の本 レナのふしぎな数学の旅
  「最短経路」を題材にした読み物で、難しい理論を知らなくても読むことが
  できます。本章でも最短パスの計算を簡単に紹介しましたが、
  この本を読めばより理解が深まるでしょう。

