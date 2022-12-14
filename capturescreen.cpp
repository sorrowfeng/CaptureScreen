#include "capturescreen.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QMouseEvent>
#include <QDebug>
#include <QScreen>
#include <QDialog>
#include <QFormLayout>
#include <QToolButton>
#include <QTimer>
#include <QTime>
#include <QStyle>

#define qout qDebug() << "[" << __FILE__ << ":" << __LINE__ << "]\t"

//#define MAC_PC

// 矩形选中区边框宽度;
#define SELECT_RECT_BORDER_WIDTH                1

// 选中矩形8个拖拽点小矩形的宽高;
#define STRETCH_RECT_WIDTH 6
#define STRETCH_RECT_HEIGHT 6

CaptureScreen::CaptureScreen(QRect rect, QWidget *parent)
    : QWidget(parent)
    , m_currentCaptureState(InitCapture)
{
    this->captureRect = rect;
    initWindow();
    initStretchRect();
    loadBackgroundPixmap();
}

CaptureScreen::~CaptureScreen()
{

}
int scaleUI(int spec)
{
    double rate = 0;
    QList<QScreen*> screens = QApplication::screens();
    if (screens.size() > 0) {
        QScreen* screen = screens[0];
        double dpi = screen->logicalDotsPerInch();
        rate = dpi / 96.0;

        if (rate < 1.1) {
            rate = 1.0;
        } else if (rate < 1.4) {
            rate = 1.25;
        } else if (rate < 1.6) {
            rate = 1.5;
        } else if (rate < 1.8) {
            rate = 1.75;
        } else {
            rate = 2.0;
        }
    }
    return int(spec * rate);
}
// finish dialog
void CaptureScreen::finishDialog()
{
    dialog = new QDialog(this);
    dialog->setWindowFlags(Qt::FramelessWindowHint);
    QHBoxLayout form(dialog);

    QToolButton *btn1 = new QToolButton(dialog);
    QToolButton *btn2 = new QToolButton(dialog);

    QStyle* style = QApplication::style();
    QIcon iconYes = style->standardIcon(QStyle::SP_DialogApplyButton);
    QIcon iconNo = style->standardIcon(QStyle::SP_DialogCloseButton);

    btn1->setIcon(iconYes);
    btn1->setIconSize(QSize(30, 30));
    btn2->setIcon(iconNo);
    btn2->setIconSize(QSize(30, 30));
    btn1->setStyleSheet("QToolButton{border:0px}");
    btn2->setStyleSheet("QToolButton{border:0px}");

    form.addWidget(btn1);
    form.addWidget(btn2);

    dialog->setCursor(Qt::PointingHandCursor);


    connect(btn1, &QToolButton::clicked, this, &CaptureScreen::finishScreenshot);
    connect(btn2, &QToolButton::clicked, this, &CaptureScreen::exitScreenshot);

    dialog->show();
}
void CaptureScreen::initWindow()
{
    this->setMouseTracking(true);
    // 由于存在类似QQ这样界面始终显示在最顶层，设置属性 Qt::WindowStaysOnTopHint;
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    this->setWindowState(Qt::WindowActive | Qt::WindowFullScreen);
}

void CaptureScreen::initStretchRect()
{
    m_stretchRectState = NotSelect;
    m_topLeftRect = QRect(0, 0, 0, 0);
    m_topRightRect = QRect(0, 0, 0, 0);
    m_bottomLeftRect = QRect(0, 0, 0, 0);
    m_bottomRightRect = QRect(0, 0, 0, 0);

    m_leftCenterRect = QRect(0, 0, 0, 0);
    m_topCenterRect = QRect(0, 0, 0, 0);
    m_rightCenterRect = QRect(0, 0, 0, 0);
    m_bottomCenterRect = QRect(0, 0, 0, 0);

}

// 获取当前屏幕图片;
void CaptureScreen::loadBackgroundPixmap()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    m_loadPixmap = screen->grabWindow(QApplication::desktop()->winId()); //抓取当前屏幕的图片
    m_screenwidth = m_loadPixmap.width();
    m_screenheight = m_loadPixmap.height();
}

void CaptureScreen::mousePressEvent(QMouseEvent *event)
{
    m_stretchRectState = getStrethRectState(event->pos());
    if (event->button() == Qt::LeftButton)
    {
        if (m_currentCaptureState == InitCapture)
        {
            m_currentCaptureState = BeginCaptureImage;
            m_beginPoint = event->pos();
        }
        // 是否在拉伸的小矩形中;
        else if (m_stretchRectState != NotSelect)
        {
            m_currentCaptureState = BeginMoveStretchRect;
            // 当前鼠标在拖动选中区顶点时,设置鼠标当前状态;
            setStretchCursorStyle(m_stretchRectState);
            m_beginMovePoint = event->pos();
        }
        // 是否在选中的矩形中;
        else if (isPressPointInSelectRect(event->pos()))
        {
            m_currentCaptureState = BeginMoveCaptureArea;
            m_beginMovePoint = event->pos();
        }
    }

    // 右键退出
    if(event->button() == Qt::RightButton)
    {
        exitScreenshot();
    }

    return QWidget::mousePressEvent(event);
}

void CaptureScreen::mouseMoveEvent(QMouseEvent* event)
{
    // 限制鼠标移动范围
    if(event->pos().x()>captureRect.x()+captureRect.width()){
       QPoint right = mapToGlobal(QPoint(captureRect.x()+captureRect.width(), event->pos().y()));
       QCursor::setPos(right);
    } else if(event->pos().x()<captureRect.x()) {
       QPoint left = mapToGlobal(QPoint(captureRect.x(), event->pos().y()));
       QCursor::setPos(left);
    } else if(event->pos().y()<captureRect.y()) {
       QPoint up = mapToGlobal(QPoint(event->pos().x(), captureRect.y()));
       QCursor::setPos(up);
    } else if(event->pos().y()>captureRect.y()+captureRect.height()) {
       QPoint down = mapToGlobal(QPoint(event->pos().x(), captureRect.y()+captureRect.height()));
       QCursor::setPos(down);
    }

    if (m_currentCaptureState == BeginCaptureImage)
    {
        m_endPoint = event->pos();
        update();
    }
    else if (m_currentCaptureState == BeginMoveCaptureArea)
    {
        m_endMovePoint = event->pos();
        update();
    }
    else if (m_currentCaptureState == BeginMoveStretchRect)
    {
        m_endMovePoint = event->pos();
        update();
        // 当前鼠标在拖动选中区顶点时,在鼠标未停止移动前，一直保持鼠标当前状态;
        return QWidget::mouseMoveEvent(event);
    }
    // 根据鼠标是否在选中区域内设置鼠标样式;
    StretchRectState stretchRectState = getStrethRectState(event->pos());
    if (stretchRectState != NotSelect)
    {
        setStretchCursorStyle(stretchRectState);
    }
    else if (isPressPointInSelectRect(event->pos()))
    {
        setCursor(Qt::SizeAllCursor);
    }
    else if (!isPressPointInSelectRect(event->pos()) && m_currentCaptureState != BeginMoveCaptureArea)
    {
        setCursor(Qt::ArrowCursor);
    }
    else
    {
        setCursor(Qt::ArrowCursor);
    }

    return QWidget::mouseMoveEvent(event);
}

void CaptureScreen::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_currentCaptureState == BeginCaptureImage)
    {
        m_currentCaptureState = FinishCaptureImage;
        m_endPoint = event->pos();
        finishDialog();
        dialog->move(m_currentSelectRect.x()+m_currentSelectRect.width()-100, m_currentSelectRect.y()+m_currentSelectRect.height());
        update();
    }
    else if (m_currentCaptureState == BeginMoveCaptureArea)
    {
        m_currentCaptureState = FinishMoveCaptureArea;
        m_endMovePoint = event->pos();
        dialog->move(m_currentSelectRect.x()+m_currentSelectRect.width()-100, m_currentSelectRect.y()+m_currentSelectRect.height());
        update();
    }
    else if (m_currentCaptureState == BeginMoveStretchRect)
    {
        m_currentCaptureState = FinishMoveStretchRect;
        m_endMovePoint = event->pos();
        dialog->move(m_currentSelectRect.x()+m_currentSelectRect.width()-100, m_currentSelectRect.y()+m_currentSelectRect.height());
        update();
    }

    return QWidget::mouseReleaseEvent(event);
}

// 当前鼠标坐标是否在选取的矩形区域内;
bool CaptureScreen::isPressPointInSelectRect(QPoint mousePressPoint)
{
    QRect selectRect = getRect(m_beginPoint, m_endPoint);
    selectRect.setX(selectRect.x()+1);
    selectRect.setY(selectRect.y()+1);
    selectRect.setWidth(selectRect.width()-2);
    selectRect.setHeight(selectRect.height()-2);
    if (selectRect.contains(mousePressPoint))
    {
        return true;
    }

    return false;
}

void CaptureScreen::paintEvent(QPaintEvent *)
{
    m_painter.begin(this);          //进行重绘;

    QColor shadowColor = QColor(0, 0, 0, 120);                      //阴影颜色设置;
    m_painter.drawPixmap(0, 0, m_loadPixmap);                       //将背景图片画到窗体上;
//    m_painter.fillRect(m_loadPixmap.rect(), shadowColor);           //画影罩效果;
    m_painter.fillRect(captureRect, shadowColor);           //画影罩效果, 限制范围后

    switch (m_currentCaptureState)
    {
    case InitCapture:
        break;
    case BeginCaptureImage:
    case FinishCaptureImage:
    case BeginMoveCaptureArea:
    case FinishMoveCaptureArea:
    case BeginMoveStretchRect:
    case FinishMoveStretchRect:
         m_currentSelectRect = getSelectRect();
         drawCaptureImage();
    case FinishCapture:
        break;
    default:
        break;
    }

    m_painter.end();  //重绘结束;
}

// 根据当前截取状态获取当前选中的截图区域;
QRect CaptureScreen::getSelectRect()
{
    QRect selectRect(0, 0, 0, 0);
    if (m_currentCaptureState == BeginCaptureImage || m_currentCaptureState == FinishCaptureImage)
    {
        selectRect = getRect(m_beginPoint, m_endPoint);
    }
    else if (m_currentCaptureState == BeginMoveCaptureArea || m_currentCaptureState == FinishMoveCaptureArea)
    {
        selectRect = getMoveRect();
    }
    else if (m_currentCaptureState == BeginMoveStretchRect || m_currentCaptureState == FinishMoveStretchRect)
    {
        selectRect = getStretchRect();
    }

    if (m_currentCaptureState == FinishCaptureImage || m_currentCaptureState == FinishMoveCaptureArea || m_currentCaptureState == FinishMoveStretchRect)
    {
        m_currentCaptureState = FinishCapture;
    }

    return selectRect;
}

// 绘制当前选中的截图区域;
void CaptureScreen::drawCaptureImage()
{
    m_capturePixmap = m_loadPixmap.copy(m_currentSelectRect);
    m_painter.drawPixmap(m_currentSelectRect.topLeft(), m_capturePixmap);
    m_painter.setPen(QPen(QColor(0, 180, 255), SELECT_RECT_BORDER_WIDTH));    //设置画笔;
    m_painter.drawRect(m_currentSelectRect);
    drawStretchRect();
}

// 绘制选中矩形各拖拽点小矩形;
void CaptureScreen::drawStretchRect()
{
    QColor color = QColor(0, 174, 255);
    // 四个角坐标;
    QPoint topLeft = m_currentSelectRect.topLeft();
    QPoint topRight = m_currentSelectRect.topRight();
    QPoint bottomLeft = m_currentSelectRect.bottomLeft();
    QPoint bottomRight = m_currentSelectRect.bottomRight();
    // 四条边中间点坐标;
    QPoint leftCenter = QPoint(topLeft.x(), (topLeft.y() + bottomLeft.y()) / 2);
    QPoint topCenter = QPoint((topLeft.x() + topRight.x()) / 2, topLeft.y());
    QPoint rightCenter = QPoint(topRight.x(), leftCenter.y());
    QPoint bottomCenter = QPoint(topCenter.x(), bottomLeft.y());

    m_topLeftRect = QRect(topLeft.x() - STRETCH_RECT_WIDTH / 2, topLeft.y() - STRETCH_RECT_HEIGHT / 2, STRETCH_RECT_WIDTH, STRETCH_RECT_HEIGHT);
    m_topRightRect = QRect(topRight.x() - STRETCH_RECT_WIDTH / 2, topRight.y() - STRETCH_RECT_HEIGHT / 2, STRETCH_RECT_WIDTH, STRETCH_RECT_HEIGHT);
    m_bottomLeftRect = QRect(bottomLeft.x() - STRETCH_RECT_WIDTH / 2, bottomLeft.y() - STRETCH_RECT_HEIGHT / 2, STRETCH_RECT_WIDTH, STRETCH_RECT_HEIGHT);
    m_bottomRightRect = QRect(bottomRight.x() - STRETCH_RECT_WIDTH / 2, bottomRight.y() - STRETCH_RECT_HEIGHT / 2, STRETCH_RECT_WIDTH, STRETCH_RECT_HEIGHT);

    m_leftCenterRect = QRect(leftCenter.x() - STRETCH_RECT_WIDTH / 2, leftCenter.y() - STRETCH_RECT_HEIGHT / 2, STRETCH_RECT_WIDTH, STRETCH_RECT_HEIGHT);
    m_topCenterRect = QRect(topCenter.x() - STRETCH_RECT_WIDTH / 2, topCenter.y() - STRETCH_RECT_HEIGHT / 2, STRETCH_RECT_WIDTH, STRETCH_RECT_HEIGHT);
    m_rightCenterRect = QRect(rightCenter.x() - STRETCH_RECT_WIDTH / 2, rightCenter.y() - STRETCH_RECT_HEIGHT / 2, STRETCH_RECT_WIDTH, STRETCH_RECT_HEIGHT);
    m_bottomCenterRect = QRect(bottomCenter.x() - STRETCH_RECT_WIDTH / 2, bottomCenter.y() - STRETCH_RECT_HEIGHT / 2, STRETCH_RECT_WIDTH, STRETCH_RECT_HEIGHT);

    m_painter.fillRect(m_topLeftRect, color);
    m_painter.fillRect(m_topRightRect, color);
    m_painter.fillRect(m_bottomLeftRect, color);
    m_painter.fillRect(m_bottomRightRect, color);
    m_painter.fillRect(m_leftCenterRect, color);
    m_painter.fillRect(m_topCenterRect, color);
    m_painter.fillRect(m_rightCenterRect, color);
    m_painter.fillRect(m_bottomCenterRect, color);
}

void CaptureScreen::keyPressEvent(QKeyEvent *event)
{
    // Esc 键退出截图;
    if (event->key() == Qt::Key_Escape)
    {
        exitScreenshot();
    }
    // Eeter键完成截图;
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
        if(m_currentSelectRect.isEmpty()) exitScreenshot();
        else finishScreenshot();
    }
}

void CaptureScreen::delay_ms(int msec)
{
    QTime dieTime = QTime::currentTime().addMSecs(msec);
    while( QTime::currentTime() < dieTime )
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}
void CaptureScreen::exitScreenshot()
{
    if(!m_currentSelectRect.isEmpty()){
        dialog->close();
        delete dialog;
    }
#ifdef MACOS
    this->showNormal();
    delay_ms(1500);
#endif
    this->close();
}
void CaptureScreen::finishScreenshot()
{
    if(!m_currentSelectRect.isEmpty()){
        dialog->close();
        delete dialog;
    }
    emit signalCompleteCature(m_capturePixmap, (m_endPoint+m_beginPoint)/2);
#ifdef MACOS
        this->showNormal();
    delay_ms(1500);
#endif
    this->close();
}

// 根据beginPoint , endPoint 获取当前选中的矩形;
QRect CaptureScreen::getRect(const QPoint &beginPoint, const QPoint &endPoint)
{
    int x, y, width, height;
    width = qAbs(beginPoint.x() - endPoint.x());
    height = qAbs(beginPoint.y() - endPoint.y());
    x = beginPoint.x() < endPoint.x() ? beginPoint.x() : endPoint.x();
    y = beginPoint.y() < endPoint.y() ? beginPoint.y() : endPoint.y();

    QRect selectedRect = QRect(x, y, width, height);
    // 避免宽或高为零时拷贝截图有误;
    // 可以看QQ截图，当选取截图宽或高为零时默认为2;
    if (selectedRect.width() == 0)
    {
        selectedRect.setWidth(1);
    }
    if (selectedRect.height() == 0)
    {
        selectedRect.setHeight(1);
    }

    return selectedRect;
}

// 获取移动后,当前选中的矩形;
QRect CaptureScreen::getMoveRect()
{
    // 通过getMovePoint方法先检查当前是否移动超出屏幕;
    QPoint movePoint = getMovePoint();
    QPoint beginPoint = m_beginPoint + movePoint;
    QPoint endPoint = m_endPoint + movePoint;
    // 结束移动选区时更新当前m_beginPoint , m_endPoint,防止下一次操作时截取的图片有问题;
    if (m_currentCaptureState == FinishMoveCaptureArea)
    {
        m_beginPoint = beginPoint;
        m_endPoint = endPoint;
        m_beginMovePoint = QPoint(0, 0);
        m_endMovePoint = QPoint(0, 0);
    }
    return getRect(beginPoint, endPoint);
}

QPoint CaptureScreen::getMovePoint()
{
    QPoint movePoint = m_endMovePoint - m_beginMovePoint;
    QRect currentRect = getRect(m_beginPoint, m_endPoint);
    // 检查当前是否移动超出屏幕;

    //移动选区是否超出屏幕左边界;
    if (currentRect.topLeft().x() + movePoint.x() < 0)
    {
        movePoint.setX(0 - currentRect.topLeft().x());
    }
    //移动选区是否超出屏幕上边界;
    if (currentRect.topLeft().y() + movePoint.y() < 0)
    {
        movePoint.setY(0 - currentRect.topLeft().y());
    }
    //移动选区是否超出屏幕右边界;
    if (currentRect.bottomRight().x() + movePoint.x() > m_screenwidth)
    {
        movePoint.setX(m_screenwidth - currentRect.bottomRight().x());
    }
    //移动选区是否超出屏幕下边界;
    if (currentRect.bottomRight().y() + movePoint.y() > m_screenheight)
    {
        movePoint.setY(m_screenheight - currentRect.bottomRight().y());
    }

    return movePoint;
}

// 获取当前鼠标位于哪一个拖拽顶点;
StretchRectState CaptureScreen::getStrethRectState(QPoint point)
{
    StretchRectState stretchRectState = NotSelect;
    if (m_topLeftRect.contains(point))
    {
        stretchRectState = TopLeftRect;
    }
    else if (m_topCenterRect.contains(point))
    {
        stretchRectState = TopCenterRect;
    }
    else if (m_topRightRect.contains(point))
    {
        stretchRectState = TopRightRect;
    }
    else if (m_rightCenterRect.contains(point))
    {
        stretchRectState = RightCenterRect;
    }
    else if (m_bottomRightRect.contains(point))
    {
        stretchRectState = BottomRightRect;
    }
    else if (m_bottomCenterRect.contains(point))
    {
        stretchRectState = BottomCenterRect;
    }
    else if (m_bottomLeftRect.contains(point))
    {
        stretchRectState = BottomLeftRect;
    }
    else if (m_leftCenterRect.contains(point))
    {
        stretchRectState = LeftCenterRect;
    }

    return stretchRectState;
}

// 设置鼠标停在拖拽定点处的样式;
void CaptureScreen::setStretchCursorStyle(StretchRectState stretchRectState)
{
    switch (stretchRectState)
    {
    case NotSelect:
        setCursor(Qt::ArrowCursor);
        break;
    case TopLeftRect:
    case BottomRightRect:
        setCursor(Qt::SizeFDiagCursor);
        break;
    case TopRightRect:
    case BottomLeftRect:
        setCursor(Qt::SizeBDiagCursor);
        break;
    case LeftCenterRect:
    case RightCenterRect:
        setCursor(Qt::SizeHorCursor);
        break;
    case TopCenterRect:
    case BottomCenterRect:
        setCursor(Qt::SizeVerCursor);
        break;
    default:
        break;
    }
}

// 获取拖拽后的矩形选中区域;
QRect CaptureScreen::getStretchRect()
{
    QRect stretchRect;
    QRect currentRect = getRect(m_beginPoint, m_endPoint);
    switch (m_stretchRectState)
    {
    case NotSelect:
        stretchRect = getRect(m_beginPoint, m_endPoint);
        break;
    case TopLeftRect:
        {
            stretchRect = getRect(currentRect.bottomRight(), m_endMovePoint);
        }
        break;
    case TopRightRect:
        {
            QPoint beginPoint = QPoint(currentRect.topLeft().x(), m_endMovePoint.y());
            QPoint endPoint = QPoint(m_endMovePoint.x(), currentRect.bottomRight().y());
            stretchRect = getRect(beginPoint, endPoint);
        }
        break;
    case BottomLeftRect:
        {
            QPoint beginPoint = QPoint(m_endMovePoint.x() , currentRect.topLeft().y());
            QPoint endPoint = QPoint(currentRect.bottomRight().x(), m_endMovePoint.y());
            stretchRect = getRect(beginPoint, endPoint);
        }
        break;
    case BottomRightRect:
        {
            stretchRect = getRect(currentRect.topLeft(), m_endMovePoint);
        }
        break;
    case LeftCenterRect:
        {
            QPoint beginPoint = QPoint(m_endMovePoint.x(), currentRect.topLeft().y());
            stretchRect = getRect(beginPoint, currentRect.bottomRight());
        }
        break;
    case TopCenterRect:
    {
        QPoint beginPoint = QPoint(currentRect.topLeft().x(), m_endMovePoint.y());
        stretchRect = getRect(beginPoint, currentRect.bottomRight());
    }
        break;
    case RightCenterRect:
    {
        QPoint endPoint = QPoint(m_endMovePoint.x(), currentRect.bottomRight().y());
        stretchRect = getRect(currentRect.topLeft(), endPoint);
    }
        break;
    case BottomCenterRect:
    {
        QPoint endPoint = QPoint(currentRect.bottomRight().x(), m_endMovePoint.y());
        stretchRect = getRect(currentRect.topLeft(), endPoint);
    }
        break;
    default:
    {
        stretchRect = getRect(m_beginPoint , m_endPoint );
    }
        break;
    }

    // 拖动结束更新 m_beginPoint , m_endPoint;
    if (m_currentCaptureState == FinishMoveStretchRect)
    {
        m_beginPoint = stretchRect.topLeft();
        m_endPoint = stretchRect.bottomRight();
    }

    return stretchRect;
}
