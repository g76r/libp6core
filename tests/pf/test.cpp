#include "pf/pfnode.h"
#include <QDateTime>
#include "pf/pfparser.h"
#include <QFile>
#include "format/csvformatter.h"

int main(void) {
  PfNode j;
  j = { "a", "b"};
  qDebug().noquote() << "j:" << j.as_pf() << j.fragments_count();
  PfNode k("k", "2");
  k.append_text_fragment("4  4 "); // should be merged with "2" as "2 4  4 "
  k.append_child({"l","o"});
  k.append_text_fragment("8"); // should not be merged with "24"
  qDebug().noquote() << "k:" << k.as_pf() << k.fragments_count();
  PfNode i { "root", "abéc€d🥨e", {
      { "child1", "a" },
      { "child2", "b", {
          { "grandchild3", "d" },
        }
      },
      { "child1", "c" },
             } };
  // qDebug() << sizeof(PfNode) << sizeof (PfNode::Fragment)
  //          << sizeof (PfNode::Fragment::_flags) << sizeof (PfNode::Fragment::_content)
  //          << sizeof(PfOptions)
  //          << sizeof(Utf8String) << sizeof(QByteArray)
  //          << sizeof(QString) << sizeof(QDateTime)
  //          << sizeof (PfNode::_fragments);
  qDebug().noquote() << i.fragments_count()
                     << i.as_pf(PfOptions().with_comments()) << i["child1"]
                        << i.content_as_text();
  i.set_attribute("child1", "A");
  qDebug().noquote() << i.fragments_count()
                     << i.as_pf(PfOptions().with_comments());
  i.set_attribute("child2", "B");
  qDebug().noquote() << i.fragments_count()
                     << i.as_pf(PfOptions().with_comments());
  i.set_attribute("child1", "C");
  qDebug().noquote() << i.fragments_count()
                     << i.as_pf(PfOptions().with_comments());
  qDebug().noquote() << PfNode().as_pf(PfOptions().with_comments());
  PfParser parser;
  QFile file;
  file.setFileName("./sample1.pf");
  file.open(QIODevice::ReadOnly);
  qDebug() << "parsing" << parser.parse(&file, PfOptions().with_comments());
  for (auto i: parser.root().children()) {
    qDebug().noquote() << i.fragments_count()
                       << i.as_pf(PfOptions().with_comments());
    qDebug() << (i["child11"])
             << i.content_as_text()
             << (i["child14"])
             << i.first_child("child14").content_as_text_pair()
             << i.first_child("unknown").content_as_text()
             << (i["unknown"]);
    qDebug().noquote() << i.fragments_count()
                       << i.as_pf();
    qDebug().noquote() << i.as_pf(PfOptions().with_indent(2).with_comments());
    qDebug().noquote() << i.as_pf(PfOptions().with_indent(4));
  }
  qDebug().noquote() << parser.root().as_pf(PfOptions().with_indent(2)
                                            .with_comments());
  file.close();
  file.setFileName("./sample2.pf");
  file.open(QIODevice::ReadOnly);
  qDebug() << "parsing" << parser.parse(&file, PfOptions().with_comments());
  for (auto i: parser.root().children()) {
    qDebug().noquote() << i.fragments_count()
                       << i.as_pf(PfOptions().with_comments().with_indent(2));
    qDebug().noquote()
        << "arraychild1:" << i.first_child("arraychild1").content_as_text();
    qDebug().noquote()
        << "arraychild2:" << i.first_child("arraychild2").content_as_text();
  }

  file.close();
  file.setFileName("./sample3.pf");
  file.open(QIODevice::ReadOnly);
  qDebug().noquote()
      << "parsing" << parser.parse(&file, PfOptions().with_comments())
      << parser.root().first_child().as_pf(PfOptions().with_comments());
  file.seek(0);
  qDebug().noquote()
      << "parsing" << parser.parse(&file)
      << parser.root().first_child().as_pf(PfOptions().with_comments());

  file.close();
  file.setFileName("./sample4.pf");
  file.open(QIODevice::ReadOnly);
  auto options = PfOptions()
                 .with_root_parsing_policy(PfOptions::FailAtSecondRootNode);
  qDebug().noquote()
      << "parsing" << parser.parse(&file, options)
      << parser.root().first_child().as_pf() << parser.root().children_count();
  file.seek(0);
  options.with_root_parsing_policy(PfOptions::StopAfterFirstRootNode);
  qDebug().noquote()
      << "parsing" << parser.parse(&file, options)
      << parser.root().first_child().as_pf() << parser.root().children_count();
  file.seek(0);
  options.with_root_parsing_policy(PfOptions::ParseEveryRootNode);
  qDebug().noquote()
      << "parsing" << parser.parse(&file, options)
      << parser.root().first_child().as_pf() << parser.root().children_count();

  qDebug().noquote()
      << PfNode{"root", "text", { {"child", "long text" } } }
         .as_pf(PfOptions().with_heretext_trigger_size(5));
  qDebug().noquote()
      << PfNode{"root", "text", { {"child", "long text" } } }
         .as_pf(PfOptions().with_heretext_trigger_size(0));
  qDebug().noquote()
      << PfNode{"root", "text", { {"child", "long text" } } }
         .as_pf(PfOptions().without_heretext_trigger_size());
  qDebug().noquote()
      << PfNode{"root", "text", { {"child", "long text" } } }
         .as_pf(PfOptions().with_heretext_trigger_size(5).with_indent(2));
  qDebug().noquote()
      << PfNode{"root", "EOF", { {"child", "EOFEOF0EOF1EOF2EOF3EOF4EOF5EOF6EOF7"
                                           "EOF8EOF9" } } }
         .as_pf(PfOptions().with_heretext_trigger_size(0));

  auto pf1 = PfNode{"root", "text", {
             PfNode{"child"}.append_loaded_binary_fragment("§", "hex:zlib") } }
             .as_pf(PfOptions().with_indent(2));
  qDebug().noquote()
      << pf1 << parser.parse(pf1)
      << parser.root().first_child().as_pf(PfOptions().with_indent(2));
  auto pf1n = parser.root().first_child();
  pf1n.set_wrappings("hex");
  qDebug().noquote() << pf1n.as_pf(PfOptions().with_indent(2));
  pf1n.set_wrappings("");
  qDebug().noquote() << pf1n.as_pf(PfOptions().with_indent(2));
  qDebug().noquote() << pf1n.as_pf(PfOptions().with_indent(2)
                                   .with_allow_bare_binary());
  pf1n.append_text_fragment("foo");
  qDebug().noquote() << pf1n.as_pf(PfOptions().with_indent(2)
                                   .with_children_first());
  qDebug().noquote() << pf1n.as_pf(PfOptions().with_indent(2)
                                   .with_payload_first());

  auto pf2n = PfNode{"root", "text", {
             PfNode{"child"}.append_loaded_binary_fragment(
               "🌞"_u8.repeated(60), "base64") } };
  qDebug().noquote() << pf2n.as_pf(PfOptions().with_indent(2));
  pf2n.set_wrappings("hex");
  qDebug().noquote() << pf2n.as_pf(PfOptions().with_indent(2));
  pf2n.append_loaded_binary_fragment(QByteArray("\0\0\0\0", 4), "hex");
  qDebug().noquote() << pf2n.as_pf(PfOptions().with_indent(2));
  pf2n.set_wrappings("");
  qDebug().noquote() << pf2n.as_pf(PfOptions().with_indent(2)
                                   .with_allow_bare_binary()).toHex();

  file.close();
  file.setFileName("./sample5.pf");
  file.open(QIODevice::ReadOnly);
  options = PfOptions().with_defer_binary_loading()
            .with_allow_bare_binary()
            .with_deferred_loading_min_size(0)
            .with_should_cache_deferred_loading(false)
            ;
  qDebug().noquote()
      << "parsing ./sample5.pf" << (parser.parse(&file, options)|"ok")
      << parser.root().first_child().fragments_count()
      << parser.root().first_child().content_as_text()
      << parser.root().first_child().content_as_binary()
      << parser.root().as_pf(options)
      << parser.root().first_child().line()
      << parser.root().first_child().column()
      << parser.root().first_child().position()
      << parser.root().first_child().first_child().position();

  file.close();
  file.setFileName("./sample6.pf");
  file.open(QIODevice::ReadOnly);
  options = PfOptions().with_defer_binary_loading()
            .with_allow_bare_binary()
            .with_deferred_loading_min_size(0)
            .with_should_cache_deferred_loading(false)
            ;
  qDebug().noquote()
      << "parsing ./sample6.pf" << (parser.parse(&file, options)|"ok")
      << parser.root().as_pf(options)
      << parser.root().first_child().position()
      << parser.root().first_child().first_child().position();

  for (int i = 100; i < 200; ++i) {
    file.close();
    file.setFileName("./sample"+Utf8String::number(i)+".pf");
    if (!file.exists())
      break;
    file.open(QIODevice::ReadOnly);
    qDebug().noquote()
        << "parsing" << file.fileName()
        << (parser.parse(&file, PfOptions().with_comments()
                         .with_defer_binary_loading()
                         .with_deferred_loading_min_size(0)) | "ok" )
        << parser.root().first_child().position()
        << parser.root().first_child().as_pf(PfOptions().with_comments());
  }

  return 0;
}
