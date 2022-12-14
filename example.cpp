QPoint p = QWidget::mapToGlobal(this->rect().topLeft());
QRect captrueRect(p.x(), p.y(), this->rect().width(), this->rect().height());
CaptureScreen *cs = new CaptureScreen(captrueRect);
connect(cs, &CaptureScreen::signalCompleteCature, this, [=](const QPixmap& pix, const QPoint& pt)
{
	// to do something
});
cs->show();
