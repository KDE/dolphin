class HTMLExporter : private KBookmarkGroupTraverser {
public:
   HTMLExporter();
   void write(const KBookmarkGroup &, const QString &);
private:
   virtual void visit(const KBookmark &);
   virtual void visitEnter(const KBookmarkGroup &);
   virtual void visitLeave(const KBookmarkGroup &);
private:
   QString m_string;
   QTextStream m_out;
   int m_level;
};

HTMLExporter::HTMLExporter() 
   : m_out(&m_string, IO_WriteOnly) {
   m_level = 0;
}

void HTMLExporter::write(const KBookmarkGroup &grp, const QString &filename) {
   HTMLExporter exporter;
   QFile file(filename);
   if (!file.open(IO_WriteOnly)) {
      kdError(7043) << "Can't write to file " << filename << endl;
      return;
   }
   QTextStream fstream(&file);
   fstream.setEncoding(QTextStream::UnicodeUTF8);
   traverse(grp);
   fstream << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">" << endl;
   fstream << "<HTML><HEAD><TITLE>My Bookmarks</TITLE></HEAD>" << endl;
   fstream << "<BODY>" << endl;
   fstream << m_string;
   fstream << "</BODY></HTML>" << endl;
}

void HTMLExporter::visit(const KBookmark &bk) {
   // kdDebug() << "visit(" << bk.text() << ")" << endl;
   m_out << "<A href=\"" << bk.url().url().utf8() << "\">";
   m_out << bk.fullText() << "</A><BR>" << endl;
}

void HTMLExporter::visitEnter(const KBookmarkGroup &grp) {
   // kdDebug() << "visitEnter(" << grp.text() << ")" << endl;
   m_out << "<H3>" << grp.fullText() << "</H3>" << endl;
   m_out << "<P style=\"margin-left: " 
         << (m_level * 3) << "em\">" << endl;
   m_level++;
} 

void HTMLExporter::visitLeave(const KBookmarkGroup &) {
   // kdDebug() << "visitLeave()" << endl;
   m_out << "</P>" << endl;
   m_level--;
   if (m_level != 0)
      m_out << "<P style=\"left-margin: " 
            << (m_level * 3) << "em\">" << endl;
}

