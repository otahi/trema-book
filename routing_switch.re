= ルーティングスイッチ

ここまでの章では、主に一台の OpenFlow スイッチを制御する方法について
学んできました。本章では、複数台の OpenFlow スイッチをまとめて制御する
アプリケーションであるルーティングスイッチを紹介します。

== 複数のスイッチを扱う

ルーティングスイッチは、X 章で扱ったラーニングスイッチと同様に
普通の L2 スイッチとして動作します。
ルーティングスイッチは、複数台の OpenFlow スイッチが連動して、
L2 スイッチ機能を提供します(@<img>{routing_switch})。

//image[routing_switch][ルーティングスイッチ]

=== トポロジーを検出する

ネットワークトポロジの検出には，OpenFlow で標準的な
Link Layer Discovery Protocol（LLDP）を用います（@<img>{lldp}）。
Topology から packet_out により送信された LLDP パケットは，
LLDP が到達した隣のスイッチから packet_in で Topology へと戻ります。
LLDP パケット中には，それが経由したスイッチとポート情報などが含まれています。
そのため，これを繰り返すことでネットワーク中のすべてのスイッチ間の
接続関係を知ることができます。

//image[lldp][トポロジーの検出]

=== 最短パスを計算する

@<img>{shortestpath}

//image[shortestpath][最短パス]

=== フローの設定
 
== 実行してみよう


=== 準備

ルーティングスイッチのソースコードは、Trema Apps にて公開されています。
まず Trema Apps を @<tt>{git} を使って取得しましょう。

//cmd{
$ git clone https://github.com/trema/apps.git
//}

Trema Apps にはさまざまなアプリケーションが含まれていますが、
今回使用するのは @<tt>{topology} と @<tt>{routing_switch} です。
この二つを順に @<tt>{make} します。

//cmd{
$ (cd apps/topology/; make)
$ (cd apps/routing_switch; make)
//}

=== ルーティングスイッチを動かす

それでは、ルーティングスイッチを動かしてみましょう。
ソースコード一式の中に @<tt>{routing_switch_fullmesh.conf} という
ファイルが含まれています。

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

今回はこのファイルを使って、以下のように起動してください。

//cmd{
$ cd ./trema
$ ./trema run -c ../apps/routing_switch/routing_switch_fullmesh.conf -d
//}

@<img>{fullmesh} のようなネットワークが構成されます。

//image[fullmesh][ネットワーク構成]

=== 見つけたリンクを表示する

@<tt>{topology} モジュールには、検出したリンクを表示するコマンドが
用意されていますので、使ってみましょう。
以下のように実行してください。

//cmd{
$ TREMA_HOME=. ../apps/topology/show_topology -D
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
$ ./trema send_packets --source host1 --dest host2
$ ./trema send_packets --source host2 --dest host1
//}

//cmd{
$ TREMA_HOME=. ../apps/flow_dumper/flow_dumper
[0x000000000000e1] table_id = 0, priority = 65535, cookie = 0xbd100000000000e,\
  ...									      \
  dl_src = 00:00:00:01:00:02, dl_dst = 00:00:00:01:00:01, dl_vlan = 65535,    \
  ...	   		      	       			  	    	      \
  actions = [output: port=3 max_len=65535]
[0x000000000000e0] table_id = 0, priority = 65535, cookie = 0xbd100000000000d,\
  ...									      \
  dl_src = 00:00:00:01:00:02, dl_dst = 00:00:00:01:00:01, dl_vlan = 65535,    \
  ...									      \
  actions = [output: port=3 max_len=65535]
//}

== まとめ/参考文献


