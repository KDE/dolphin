/*
 * SPDX-FileCopyrightText: 2026 Chinmoy Pradhan <chinmoy@snaptrude.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// Measures the wall-clock cost of painting a Dolphin file view while scrolling,
// and how much of that cost is KStandardItemListWidget::triggerCacheRefreshing().
// Not run automatically by ctest; build target `kitemlistviewpaintbenchmark`.

#include "kitemviews/kfileitemmodel.h"
#include "kitemviews/kitemlistcontainer.h"
#include "kitemviews/kitemlistcontroller.h"
#include "testdir.h"
#include "views/dolphinitemlistview.h"

#include <QElapsedTimer>
#include <QGraphicsScene>
#include <QIcon>
#include <QImage>
#include <QPainter>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

#include <algorithm>

namespace
{
constexpr int FileCount = 3000;
constexpr int ViewWidth = 1400;
constexpr int ViewHeight = 900;

struct Stats {
    double mean = 0;
    double median = 0;
    double p95 = 0;
    double max = 0;
    int over16ms = 0;
    int frames = 0;
};

Stats computeStats(QList<double> times)
{
    Stats s;
    if (times.isEmpty()) {
        return s;
    }
    std::sort(times.begin(), times.end());
    s.frames = times.size();
    for (double t : times) {
        s.mean += t;
        if (t > 16.0) {
            ++s.over16ms;
        }
    }
    s.mean /= times.size();
    s.median = times.at(times.size() / 2);
    s.p95 = times.at(std::min<int>(times.size() - 1, times.size() * 95 / 100));
    s.max = times.last();
    return s;
}

void report(const QString &label, const Stats &s)
{
    qInfo("    %-24s paint: frames=%4d  mean=%6.2f  median=%6.2f  p95=%6.2f  max=%7.2f  >16ms=%d",
          qUtf8Printable(label),
          s.frames,
          s.mean,
          s.median,
          s.p95,
          s.max,
          s.over16ms);
}
}

/** Exposes the protected widget list so the benchmark can invalidate every visible widget. */
class BenchmarkView : public DolphinItemListView
{
    Q_OBJECT
public:
    using DolphinItemListView::visibleItemListWidgets;
};

class KItemListViewPaintBenchmark : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void diagnostics();

    void scrollPaint_data();
    void scrollPaint();

    void invalidateAllPaint_data();
    void invalidateAllPaint();

    void sceneActivationPaint_data();
    void sceneActivationPaint();

private:
    /** Renders the container once and returns the elapsed milliseconds. */
    double renderOnce();
    void applyLayout(int layout);

    BenchmarkView *m_view = nullptr;
    KItemListController *m_controller = nullptr;
    KFileItemModel *m_model = nullptr;
    TestDir *m_testDir = nullptr;
    KItemListContainer *m_container = nullptr;
    QImage m_target;
};

void KItemListViewPaintBenchmark::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);

    m_testDir = new TestDir();
    QStringList files;
    files.reserve(FileCount);
    // A spread of extensions so several distinct mime icons are involved, and a
    // spread of name lengths so text eliding is exercised.
    const QStringList extensions = {"txt", "cpp", "h", "png", "pdf", "odt", "tar.gz", "mp3", "svg", ""};
    for (int i = 0; i < FileCount; ++i) {
        const QString ext = extensions.at(i % extensions.size());
        const QString padding = QString(i % 40, QLatin1Char('x'));
        files << QStringLiteral("file_%1_%2%3").arg(i, 5, 10, QLatin1Char('0')).arg(padding).arg(ext.isEmpty() ? QString() : QLatin1Char('.') + ext);
    }
    m_testDir->createFiles(files);

    m_model = new KFileItemModel();
    m_view = new BenchmarkView();
    m_controller = new KItemListController(m_model, m_view, this);
    m_container = new KItemListContainer(m_controller);
    m_controller = m_container->controller();

    m_container->resize(ViewWidth, ViewHeight);
    m_container->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_container));

    QSignalSpy spyLoaded(m_model, &KFileItemModel::directoryLoadingCompleted);
    m_model->loadDirectory(m_testDir->url());
    QVERIFY(spyLoaded.wait(30000));
    QCOMPARE(m_model->count(), FileCount);

    m_target = QImage(ViewWidth, ViewHeight, QImage::Format_ARGB32_Premultiplied);

    // Let the roles updater settle so async icon/role churn does not pollute the
    // measurements.
    QElapsedTimer settle;
    settle.start();
    [[maybe_unused]] const bool settled = QTest::qWaitFor(
        [this, &settle] {
            renderOnce();
            return settle.elapsed() > 2000;
        },
        10000);
}

void KItemListViewPaintBenchmark::cleanupTestCase()
{
    // Deliberately leaked: tearing the view down races with the roles updater's
    // worker threads and is not what this benchmark measures.
}

double KItemListViewPaintBenchmark::renderOnce()
{
    QElapsedTimer timer;
    timer.start();
    QPainter painter(&m_target);
    m_container->render(&painter);
    painter.end();
    return timer.nsecsElapsed() / 1e6;
}

void KItemListViewPaintBenchmark::applyLayout(int layout)
{
    if (layout == DolphinItemListView::DetailsLayout) {
        m_view->setVisibleRoles({"text", "size", "modificationtime", "type", "permissions", "owner"});
    } else {
        m_view->setVisibleRoles({"text"});
    }
    m_view->setItemLayout(static_cast<DolphinItemListView::ItemLayout>(layout));
    m_view->setScrollOffset(0);
    // Warm up: create and lay out the first screenful of widgets.
    for (int i = 0; i < 5; ++i) {
        renderOnce();
    }
}

void KItemListViewPaintBenchmark::diagnostics()
{
    qInfo("icon theme = '%s'", qUtf8Printable(QIcon::themeName()));
    const QIcon icon = QIcon::fromTheme(QStringLiteral("text-x-generic"));
    qInfo("QIcon::fromTheme(\"text-x-generic\").isNull() = %d, pixmap(48) = %dx%d", icon.isNull(), icon.pixmap(48, 48).width(), icon.pixmap(48, 48).height());

    const QList<std::pair<const char *, int>> layouts = {
        {"icons", DolphinItemListView::IconsLayout},
        {"compact", DolphinItemListView::CompactLayout},
        {"details", DolphinItemListView::DetailsLayout},
    };
    for (const auto &[name, layout] : layouts) {
        applyLayout(layout);
        qInfo("%-8s view=%.0fx%.0f  itemSize=%.0fx%.0f  iconSize=%d  visibleWidgets=%lld  maxScrollOffset=%.0f",
              name,
              m_view->size().width(),
              m_view->size().height(),
              m_view->itemSize().width(),
              m_view->itemSize().height(),
              m_view->styleOption().iconSize,
              qint64(m_view->visibleItemListWidgets().size()),
              m_view->maximumScrollOffset());
    }
}

void KItemListViewPaintBenchmark::scrollPaint_data()
{
    QTest::addColumn<int>("layout");
    QTest::addColumn<int>("step");
    // step is in pixels of scroll offset per rendered frame. 8px/frame is a slow
    // drag; 200px/frame is roughly a fast wheel flick.
    QTest::newRow("icons/slow") << int(DolphinItemListView::IconsLayout) << 8;
    QTest::newRow("icons/fast") << int(DolphinItemListView::IconsLayout) << 200;
    QTest::newRow("compact/slow") << int(DolphinItemListView::CompactLayout) << 8;
    QTest::newRow("compact/fast") << int(DolphinItemListView::CompactLayout) << 200;
    QTest::newRow("details/slow") << int(DolphinItemListView::DetailsLayout) << 8;
    QTest::newRow("details/fast") << int(DolphinItemListView::DetailsLayout) << 200;
}

void KItemListViewPaintBenchmark::scrollPaint()
{
    QFETCH(int, layout);
    QFETCH(int, step);

    applyLayout(layout);

    QList<double> times;
    const qreal maxOffset = m_view->maximumScrollOffset();
    for (qreal offset = 0; offset < maxOffset && times.size() < 400; offset += step) {
        m_view->setScrollOffset(offset);
        times << renderOnce();
    }

    const QString tag = QString::fromLatin1(QTest::currentDataTag());
    report(tag, computeStats(times));
}

void KItemListViewPaintBenchmark::invalidateAllPaint_data()
{
    QTest::addColumn<int>("layout");
    QTest::newRow("icons") << int(DolphinItemListView::IconsLayout);
    QTest::newRow("compact") << int(DolphinItemListView::CompactLayout);
    QTest::newRow("details") << int(DolphinItemListView::DetailsLayout);
}

void KItemListViewPaintBenchmark::invalidateAllPaint()
{
    QFETCH(int, layout);

    applyLayout(layout);

    // QGraphicsScene sends WindowActivate/WindowDeactivate to every item, and
    // KStandardItemListWidget::event() turns that into m_dirtyContent = true. So
    // alt-tabbing in or out of the window forces the next single paint to refresh
    // the cache of a whole screenful. This is the worst case behind the
    // "randomly over 16 ms" report.
    QList<double> times;
    for (int i = 0; i < 40; ++i) {
        const auto widgets = m_view->visibleItemListWidgets();
        for (KItemListWidget *widget : widgets) {
            QEvent event(i % 2 ? QEvent::WindowActivate : QEvent::WindowDeactivate);
            QCoreApplication::sendEvent(widget, &event);
        }
        times << renderOnce();
    }

    const QString tag = QString::fromLatin1(QTest::currentDataTag());
    qInfo("    %-24s visible widgets = %lld", qUtf8Printable(tag), qint64(m_view->visibleItemListWidgets().size()));
    report(tag, computeStats(times));
}

void KItemListViewPaintBenchmark::sceneActivationPaint_data()
{
    invalidateAllPaint_data();
}

/**
 * Same as invalidateAllPaint(), but the activation event is sent to the scene
 * rather than to each widget directly, i.e. it goes through the real propagation
 * path: QGraphicsScene::event() sends WindowActivate to top-level items only,
 * and QGraphicsItem::sceneEvent() forwards it down to every visible child. If
 * these paint times match invalidateAllPaint(), the propagation reaches every
 * item widget and each alt-tab really does dirty the whole screenful.
 */
void KItemListViewPaintBenchmark::sceneActivationPaint()
{
    QFETCH(int, layout);

    applyLayout(layout);
    QVERIFY(m_view->scene());

    QList<double> times;
    for (int i = 0; i < 40; ++i) {
        QEvent event(i % 2 ? QEvent::WindowActivate : QEvent::WindowDeactivate);
        QCoreApplication::sendEvent(m_view->scene(), &event);
        times << renderOnce();
    }

    report(QString::fromLatin1(QTest::currentDataTag()), computeStats(times));
}

QTEST_MAIN(KItemListViewPaintBenchmark)

#include "kitemlistviewpaintbenchmark.moc"
