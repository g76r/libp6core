#ifndef ASYNCTEXTVIEW_H
#define ASYNCTEXTVIEW_H

#include "textview.h"

/** Asynchronously updating and caching base class for text views.
 * Cache last rendered text string and calls updateText() on model updates.
 * Calls updateText() only once per event loop iteration regardless the number
 * of times update() was called, like QWidget::update() does.
 */
// LATER provide several update strategies (on change, on request, delayed change)
class LIBQTSSUSHARED_EXPORT AsyncTextView : public TextView {
  Q_OBJECT
protected:
  QString _text;

public:
  explicit AsyncTextView(QObject *parent = 0);
  QString text() const;
  /** Set which model the view will display.
   */
  void setModel(QAbstractItemModel *model);

public slots:
  /** Force update now.
   * This method can be called several times in the same event loop iteration
   * without forcing several updates, like QWidget::update() does.
   * It is automatically connected to model signals when setModel() is called,
   * therefore there should be no need to call update() explicitly.
   */
  void update();

protected:
  void customEvent(QEvent *event);
  virtual void updateText() = 0;
};

#endif // ASYNCTEXTVIEW_H
