/*!
 * \file pqHistogramChart.cxx
 *
 * \brief
 *   The pqHistogramChart class is used to display a histogram chart.
 *
 * \author Mark Richardson
 * \date   May 12, 2005
 */

#include "pqHistogramChart.h"
#include "pqHistogramColor.h"
#include "pqChartValue.h"
#include "pqChartAxis.h"
#include "pqHistogramSelection.h"

#include <QFontMetrics>
#include <QPainter>
#include <vtkstd/vector>
#include <vtkstd/list>


/// \class QHistogramItem
/// \brief
///   The QHistogramItem class stores the information needed
///   to draw an histogram bar.
class QHistogramItem
{
public:
  /// Creates a histogram bar representation.
  QHistogramItem();

  /// \brief
  ///   Creates a histogram bar representation and assigns the value.
  QHistogramItem(const pqChartValue &value);
  ~QHistogramItem() {}

  /// \brief
  ///   Sets the value for the histogram bar.
  /// \param value The value of the histogram bar.
  void setValue(const pqChartValue &value);

  /// \brief
  ///   Gets the value for the histogram bar.
  /// \return
  ///   The value for the histogram bar.
  const pqChartValue &getValue() const {return value;}

public:
  QRect Bounds;       ///< The bounding rectangle of the bar.

private:
  pqChartValue value; ///< Stores the histogram value.
};


/// \class QHistogramHighlight
/// \brief
///   The QHistogramHighlight class stores the information needed
///   to draw a highlighted range.
class QHistogramHighlight : public pqHistogramSelection
{
public:
  /// Creates an empty highlight object.
  QHistogramHighlight();

  /// \brief
  ///   Copies a selection range.
  /// \param other The selection range to copy.
  QHistogramHighlight(const pqHistogramSelection &other);

  /// \brief
  ///   Copies a selection range and bounding box.
  /// \param other The selection range to copy.
  QHistogramHighlight(const QHistogramHighlight &other);
  virtual ~QHistogramHighlight() {}

  /// \brief
  ///   Copies a selection range.
  /// \param other The selection range to copy.
  /// \return
  ///   A reference to the highlight object.
  QHistogramHighlight &operator=(const pqHistogramSelection &other);

  /// \brief
  ///   Copies a selection range and bounding box.
  /// \param other The selection range to copy.
  /// \return
  ///   A reference to the highlight object.
  QHistogramHighlight &operator=(const QHistogramHighlight &other);

public:
  QRect Bounds; ///< The bounding rectangle of the range highlight.
};


/// \class pqHistogramChartData
/// \brief
///   The pqHistogramChartData class hides the private data of the
///   pqHistogramChart class.
class pqHistogramChartData
{
public:
  pqHistogramChartData();
  ~pqHistogramChartData() {}

  void clearSelection();

public:
  /// Stores the list of histogram bars.
  vtkstd::vector<QHistogramItem *> items;

  /// Stores the list of current selection ranges.
  pqHistogramSelectionList selections;

  /// Stores the default color scheme for the histogram.
  static pqHistogramColor colorScheme;
};


QHistogramItem::QHistogramItem()
  : Bounds(), value((int)0)
{
}

QHistogramItem::QHistogramItem(const pqChartValue &val)
  : Bounds(), value(val)
{
}

void QHistogramItem::setValue(const pqChartValue &value)
{
  this->value = value;
}


QHistogramHighlight::QHistogramHighlight()
  : pqHistogramSelection(), Bounds()
{
}

QHistogramHighlight::QHistogramHighlight(const pqHistogramSelection &other)
  : pqHistogramSelection(other), Bounds()
{
}

QHistogramHighlight::QHistogramHighlight(const QHistogramHighlight &other)
  : pqHistogramSelection(other), Bounds(other.Bounds)
{
}

QHistogramHighlight &QHistogramHighlight::operator=(
    const pqHistogramSelection &other)
{
  this->setType(other.getType());
  this->setRange(other.getFirst(), other.getSecond());
  this->Bounds.setCoords(0, 0, 0, 0);
  return *this;
}

QHistogramHighlight &QHistogramHighlight::operator=(
    const QHistogramHighlight &other)
{
  this->setType(other.getType());
  this->setRange(other.getFirst(), other.getSecond());
  this->Bounds.setTopLeft(other.Bounds.topLeft());
  this->Bounds.setBottomRight(other.Bounds.bottomRight());
  return *this;
}


pqHistogramColor pqHistogramChartData::colorScheme;

pqHistogramChartData::pqHistogramChartData()
  : items(), selections()
{
}

void pqHistogramChartData::clearSelection()
{
  pqHistogramSelectionList::Iterator iter = this->selections.begin();
  for( ; iter != this->selections.end(); ++iter)
    delete *iter;

  this->selections.clear();
}


QColor pqHistogramChart::LightBlue = QColor(125, 165, 230);

pqHistogramChart::pqHistogramChart(QObject *parent)
  : QObject(parent), Bounds(), Select(LightBlue)
{
  this->Style = pqHistogramChart::Fill;
  this->OutlineType = pqHistogramChart::Darker;
  this->Colors = &pqHistogramChartData::colorScheme;
  this->Data = new pqHistogramChartData();
}

pqHistogramChart::~pqHistogramChart()
{
  this->resetData();
  if(this->Data)
    delete this->Data;
}

void pqHistogramChart::setAxes(pqChartAxis *xAxis, pqChartAxis *yAxis)
{
  this->XAxis = xAxis;
  this->YAxis = yAxis;
}

void pqHistogramChart::setData(const pqChartValueList &values,
    const pqChartValue &min, const pqChartValue &interval)
{
  bool hadSelection = hasSelection();
  this->resetData();
  if(!this->Data || values.getSize() == 0)
    return;

  if(!this->XAxis || !this->YAxis)
    return;

  pqChartValue yMin;
  pqChartValueList::ConstIterator iter = values.constBegin();
  yMin.convertTo(iter->getType());
  pqChartValue yMax = yMin;
  for( ; iter != values.constEnd(); ++iter)
    {
    QHistogramItem *item = new QHistogramItem(*iter);
    this->Data->items.push_back(item);
    if(*iter > yMax)
      yMax = *iter;
    else if(*iter < yMin)
      yMin = *iter;
    }

  // Set up the x and y axes min/max. The axis changes will cause
  // layout signals to be sent. Avoid multiple signals by blocking
  // the signals for all but the last change.
  pqChartValue xMax = min + (interval * values.getSize());
  if(this->YAxis->getLayoutType() != pqChartAxis::FixedInterval)
    {
    // Set up the extra padding parameters for the min/max.
    if(yMin == 0)
      {
      this->YAxis->setExtraMaxPadding(true);
      this->YAxis->setExtraMinPadding(false);
      }
    else if(yMax == 0)
      {
      this->YAxis->setExtraMaxPadding(false);
      this->YAxis->setExtraMinPadding(true);
      }
    else
      {
      this->YAxis->setExtraMaxPadding(true);
      this->YAxis->setExtraMinPadding(true);
      }

    this->YAxis->blockSignals(true);
    this->YAxis->setValueRange(yMin, yMax);
    this->YAxis->blockSignals(false);
    }

  this->XAxis->setInterval(interval);
  this->XAxis->setValueRange(min, xMax);

  // If there was a selection change, notify the observers.
  if(hadSelection)
    emit this->selectionChanged(this->Data->selections);
}

int pqHistogramChart::getBinCount() const
{
  if(this->Data)
    return static_cast<int>(this->Data->items.size());
  return 0;
}

int pqHistogramChart::getLastBin() const
{
  int last = this->getBinCount();
  if(last > 0)
    last--;
  return last;
}

int pqHistogramChart::getBinWidth() const
{
  if(this->Data && this->Data->items.size() > 0 && this->Bounds.isValid())
    return this->Bounds.width()/this->getBinCount();
  return 0;
}

int pqHistogramChart::getBinAt(int x, int y) const
{
  if(this->Data)
    {
    vtkstd::vector<QHistogramItem *>::iterator iter = this->Data->items.begin();
    for(int i = 0; iter != this->Data->items.end(); iter++, i++)
      {
      if(*iter && (*iter)->Bounds.isValid() && (*iter)->Bounds.contains(x, y))
        return i;
      }
    }

  return -1;
}

bool pqHistogramChart::getValueAt(int x, int y, pqChartValue &value) const
{
  if(this->Data && this->Bounds.isValid() && this->XAxis->isValid() &&
      this->Bounds.contains(x, y))
    {
    pqChartValue range = this->XAxis->getValueRange();
    if(range.getType() == pqChartValue::IntValue)
      {
      // Adjust the pick location if the pixel to value ratio is
      // bigger than one. This will help find the closest value.
      int pixel = 0;
      if(range != 0)
        pixel = this->XAxis->getPixelRange() / range;
      if(pixel < 0)
        pixel = -pixel;
      if(pixel > 1)
        x += (pixel/2) + 1;
      }

    value = this->XAxis->getValueFor(x);
    return true;
    }

  return false;
}

bool pqHistogramChart::getValueRangeAt(int x, int y,
    pqHistogramSelection &range) const
{
  if(this->Data && this->Bounds.isValid() && this->XAxis->isValid() &&
    this->Data->selections.getType() == pqHistogramSelection::Value &&
    this->Bounds.contains(x, y))
    {
    pqChartValue diff = this->XAxis->getValueRange();
    if(diff.getType() == pqChartValue::IntValue)
      {
      // Adjust the pick location if the pixel to value ratio is
      // bigger than one. This will help find the closest value.
      int pixel = 0;
      if(diff != 0)
        pixel = this->XAxis->getPixelRange() / diff;
      if(pixel < 0)
        pixel = -pixel;
      if(pixel > 1)
        x += (pixel/2) + 1;
      }

    // Search through the current selection list.
    pqChartValue value = this->XAxis->getValueFor(x);
    pqHistogramSelectionList::ConstIterator iter = this->Data->selections.begin();
    for( ; iter != this->Data->selections.end(); ++iter)
      {
      const pqHistogramSelection *selection = *iter;
      if(selection->getFirst() <= value)
        {
        if(selection->getSecond() >= value)
          {
          range.setType(selection->getType());
          range.setRange(selection->getFirst(), selection->getSecond());
          return true;
          }
        }
      else
        break;
      }
    }

  return false;
}

void pqHistogramChart::getBinsIn(const QRect &area,
    pqHistogramSelectionList &list) const
{
  if(this->Data)
    {
    pqChartValue i((int)0);
    pqHistogramSelection *selection = 0;
    vtkstd::vector<QHistogramItem *>::iterator iter = this->Data->items.begin();
    for( ; iter != this->Data->items.end(); iter++, ++i)
      {
      if(*iter && (*iter)->Bounds.isValid() &&
          (*iter)->Bounds.intersects(area))
        {
        selection = new pqHistogramSelection(i, i);
        if(selection)
          {
          selection->setType(pqHistogramSelection::Bin);
          list.pushBack(selection);
          }
        }
      }

    if(list.getSize() > 0)
      {
      pqHistogramSelectionList toDelete;
      list.sortAndMerge(toDelete);
      if(toDelete.getSize() > 0)
        {
        pqHistogramSelectionList::Iterator jter = toDelete.begin();
        for( ; jter != toDelete.end(); jter++)
          {
          if(*jter != 0)
            delete *jter;
          }
        }
      }
    }
}

void pqHistogramChart::getValuesIn(const QRect &area,
    pqHistogramSelectionList &list) const
{
  if(!this->Data || !area.isValid() || !this->Bounds.isValid())
    return;
  if(!this->XAxis->isValid() || !area.intersects(this->Bounds))
    return;

  pqChartValue left;
  pqChartValue right;
  QRect i = area.intersect(this->Bounds);
  if(getValueAt(i.left(), i.top(), left) &&
      getValueAt(i.right(), i.top(), right))
    {
    pqHistogramSelection *selection = new pqHistogramSelection(left, right);
    if(selection)
      {
      selection->setType(pqHistogramSelection::Value);
      list.pushBack(selection);

      pqHistogramSelectionList toDelete;
      list.sortAndMerge(toDelete);
      if(toDelete.getSize() > 0)
        {
        pqHistogramSelectionList::Iterator jter = toDelete.begin();
        for( ; jter != toDelete.end(); jter++)
          {
          if(*jter != 0)
            delete *jter;
          }
        }
      }
    }
}

bool pqHistogramChart::hasSelection() const
{
  if(this->Data)
    return this->Data->selections.getSize() > 0;
  return false;
}

void pqHistogramChart::getSelection(pqHistogramSelectionList &list) const
{
  if(this->Data)
    list.makeNewCopy(this->Data->selections);
}

void pqHistogramChart::selectAllBins()
{
  pqHistogramSelection selection;
  selection.setType(pqHistogramSelection::Bin);
  selection.setRange((int)0, getLastBin());
  this->setSelection(&selection);
}

void pqHistogramChart::selectAllValues()
{
  if(this->Data && this->XAxis->isValid())
    {
    pqHistogramSelection selection;
    selection.setType(pqHistogramSelection::Value);
    selection.setRange(this->XAxis->getMinValue(), this->XAxis->getMaxValue());
    this->setSelection(&selection);
    }
}

void pqHistogramChart::selectNone()
{
  if(this->Data)
    {
    this->Data->clearSelection();
    emit this->selectionChanged(this->Data->selections);
    }
}

void pqHistogramChart::selectInverse()
{
  if(this->Data && this->XAxis->isValid())
    {
    pqHistogramSelection selection;
    if(this->Data->selections.getType() == pqHistogramSelection::Value)
      {
      selection.setType(pqHistogramSelection::Value);
      selection.setRange(this->XAxis->getMinValue(),
        this->XAxis->getMaxValue());
      }
    else
      {
      selection.setType(pqHistogramSelection::Bin);
      selection.setRange((int)0, getLastBin());
      }

    this->xorSelection(&selection);
    }
}

void pqHistogramChart::setSelection(const pqHistogramSelectionList &list)
{
  if(this->Data)
    this->Data->clearSelection();
  this->addSelection(list);
}

void pqHistogramChart::addSelection(const pqHistogramSelectionList &list)
{
  if(!this->Data)
    return;

  // Create highlight items for each of the selection items.
  QHistogramHighlight *highlight = 0;
  pqHistogramSelectionList newList;
  pqHistogramSelectionList::ConstIterator iter = list.begin();
  for( ; iter != list.end(); ++iter)
    {
    if(*iter)
      highlight = new QHistogramHighlight(*(*iter));
    if(highlight)
      newList.pushBack(highlight);
    }

  pqHistogramSelectionList toDelete;
  this->Data->selections.unite(newList, toDelete);
  pqHistogramSelectionList::Iterator diter = toDelete.begin();
  for( ; diter != toDelete.end(); ++diter)
    delete *diter;

  this->layoutSelection();
  emit this->selectionChanged(this->Data->selections);
}

void pqHistogramChart::xorSelection(const pqHistogramSelectionList &list)
{
  if(!this->Data)
    return;

  // Create highlight items for each of the selection items.
  QHistogramHighlight *highlight = 0;
  pqHistogramSelectionList newList;
  pqHistogramSelectionList::ConstIterator iter = list.begin();
  for( ; iter != list.end(); ++iter)
    {
    if(*iter)
      highlight = new QHistogramHighlight(*(*iter));
    if(highlight)
      newList.pushBack(highlight);
    }

  pqHistogramSelectionList toDelete;
  this->Data->selections.Xor(newList, toDelete);
  pqHistogramSelectionList::Iterator diter = toDelete.begin();
  for( ; diter != toDelete.end(); ++diter)
    delete *diter;

  this->layoutSelection();
  emit this->selectionChanged(this->Data->selections);
}

void pqHistogramChart::subtractSelection(const pqHistogramSelectionList &list)
{
  if(!this->Data)
    return;

  // Create highlight items for each of the selection items.
  QHistogramHighlight *highlight = 0;
  pqHistogramSelectionList newList;
  pqHistogramSelectionList::ConstIterator iter = list.begin();
  for( ; iter != list.end(); ++iter)
    {
    if(*iter)
      highlight = new QHistogramHighlight(*(*iter));
    if(highlight)
      newList.pushBack(highlight);
    }

  pqHistogramSelectionList toDelete;
  this->Data->selections.subtract(newList, toDelete);
  pqHistogramSelectionList::Iterator diter = toDelete.begin();
  for( ; diter != toDelete.end(); ++diter)
    delete *diter;

  this->layoutSelection();
  emit this->selectionChanged(this->Data->selections);
}

void pqHistogramChart::setSelection(const pqHistogramSelection *range)
{
  if(this->Data)
    this->Data->clearSelection();
  this->addSelection(range);
}

void pqHistogramChart::addSelection(const pqHistogramSelection *range)
{
  if(!this->Data || !range || range->getType() == pqHistogramSelection::None)
    return;
  if(this->Data->selections.getType() != pqHistogramSelection::None &&
      this->Data->selections.getType() != range->getType())
    {
    return;
    }

  QHistogramHighlight *highlight = new QHistogramHighlight(*range);
  if(highlight)
    {
    if(highlight->getType() == pqHistogramSelection::Bin)
      {
      pqChartValue min = (int)0;
      pqChartValue max = getLastBin();
      highlight->adjustRange(min, max);
      }
    else
      {
      highlight->adjustRange(this->XAxis->getMinValue(),
          this->XAxis->getMaxValue());
      }

    pqHistogramSelectionList toDelete;
    this->Data->selections.unite(highlight, toDelete);
    pqHistogramSelectionList::Iterator iter = toDelete.begin();
    for( ; iter != toDelete.end(); ++iter)
      delete *iter;

    this->layoutSelection();
    emit this->selectionChanged(this->Data->selections);
    }
}

void pqHistogramChart::xorSelection(const pqHistogramSelection *range)
{
  if(!range || !this->Data)
    return;
  if((this->Data->selections.getType() != pqHistogramSelection::None &&
      this->Data->selections.getType() != range->getType()) ||
      range->getType() == pqHistogramSelection::None)
    {
    return;
    }

  QHistogramHighlight *highlight = new QHistogramHighlight(*range);
  if(highlight)
    {
    if(highlight->getType() == pqHistogramSelection::Bin)
      {
      pqChartValue min = (int)0;
      pqChartValue max = getLastBin();
      highlight->adjustRange(min, max);
      }
    else
      {
      highlight->adjustRange(this->XAxis->getMinValue(),
          this->XAxis->getMaxValue());
      }

    pqHistogramSelectionList toDelete;
    this->Data->selections.Xor(highlight, toDelete);
    pqHistogramSelectionList::Iterator iter = toDelete.begin();
    for( ; iter != toDelete.end(); ++iter)
      delete *iter;

    this->layoutSelection();
    emit this->selectionChanged(this->Data->selections);
    }
}

void pqHistogramChart::moveSelection(const pqHistogramSelection &range,
    const pqChartValue &offset)
{
  if(!this->Data || offset == 0)
    return;
  if((this->Data->selections.getType() != pqHistogramSelection::None &&
      this->Data->selections.getType() != range.getType()) ||
      range.getType() == pqHistogramSelection::None)
    {
    return;
    }

  // Find the given selection range and remove it from the list.
  QHistogramHighlight *highlight = 0;
  pqHistogramSelectionList::Iterator iter = this->Data->selections.begin();
  for( ; iter != this->Data->selections.end(); ++iter)
    {
    pqHistogramSelection *selection = *iter;
    if(selection->getFirst() == range.getFirst() &&
        selection->getSecond() == range.getSecond())
      {
      highlight = dynamic_cast<QHistogramHighlight *>(selection);
      if(highlight)
        this->Data->selections.erase(iter);

      break;
      }
    }

  if(highlight)
    {
    // Add the offset to the selection range. Make sure the new range
    // fits in the chart area.
    highlight->moveRange(offset);
    if(highlight->getType() == pqHistogramSelection::Bin)
      {
      pqChartValue min = (int)0;
      pqChartValue max = getLastBin();
      highlight->adjustRange(min, max);
      }
    else
      {
      highlight->adjustRange(this->XAxis->getMinValue(),
          this->XAxis->getMaxValue());
      }

    // Unite the adjusted selection to the list.
    pqHistogramSelectionList toDelete;
    this->Data->selections.unite(highlight, toDelete);
    for(iter = toDelete.begin(); iter != toDelete.end(); ++iter)
      delete *iter;

    this->layoutSelection();
    emit this->selectionChanged(this->Data->selections);
    }
}

void pqHistogramChart::setBinColorScheme(pqHistogramColor *scheme)
{
  if(!scheme && this->Colors == &pqHistogramChartData::colorScheme)
    return;

  if(this->Colors != scheme)
    {
    if(scheme)
      this->Colors = scheme;
    else
      this->Colors = &pqHistogramChartData::colorScheme;
    emit this->repaintNeeded();
    }
}

void pqHistogramChart::setBinHighlightStyle(HighlightStyle style)
{
  if(this->Style != style)
    {
    this->Style = style;
    emit this->repaintNeeded();
    }
}

void pqHistogramChart::setBinOutlineStyle(OutlineStyle style)
{
  if(this->OutlineType != style)
    {
    this->OutlineType = style;
    emit repaintNeeded();
    }
}

void pqHistogramChart::layoutChart()
{
  if(!this->Data || !this->XAxis || !this->YAxis)
    return;

  // Make sure the axes are valid.
  if(!this->XAxis->isValid() || !this->YAxis->isValid())
    return;

  // Set up the chart area based on the remaining space.
  this->Bounds.setTop(this->YAxis->getMaxPixel());
  this->Bounds.setLeft(this->XAxis->getMinPixel());
  this->Bounds.setRight(this->XAxis->getMaxPixel());
  this->Bounds.setBottom(this->YAxis->getMinPixel());

  // Set up the size for each bar in the histogram.
  int i = 0;
  int bottom = this->YAxis->getMinPixel();
  bool reversed = false;
  if(this->YAxis->isZeroInRange())
    {
    pqChartValue zero(0);
    zero.convertTo(this->YAxis->getMaxValue().getType());
    bottom = this->YAxis->getPixelFor(zero);
    }
  else if(this->YAxis->getValueRange() < 0)
    {
    bottom = this->YAxis->getMaxPixel();
    reversed = true;
    }

  int total = this->XAxis->getNumberShowing();
  pqChartValue v = this->XAxis->getMinValue();
  vtkstd::vector<QHistogramItem *>::iterator iter = this->Data->items.begin();
  for( ; iter != this->Data->items.end() && i < total; iter++, i++)
    {
    QHistogramItem *item = *iter;
    item->Bounds.setLeft(this->XAxis->getPixelFor(v));
    v += this->XAxis->getInterval();
    item->Bounds.setRight(this->XAxis->getPixelFor(v));
    if(reversed || item->getValue() < 0)
      {
      item->Bounds.setTop(bottom);
      item->Bounds.setBottom(this->YAxis->getPixelFor(item->getValue()));
      }
    else
      {
      item->Bounds.setTop(this->YAxis->getPixelFor(item->getValue()));
      item->Bounds.setBottom(bottom);
      }
    }

  this->layoutSelection();
}

void pqHistogramChart::drawBackground(QPainter *p, const QRect &area)
{
  if(!this->Data || !p || !p->isActive() || !area.isValid())
    return;
  if(!this->Bounds.isValid())
    return;

  // Draw in the selection if there is one.
  QHistogramHighlight *highlight = 0;
  pqHistogramSelectionList::Iterator siter = this->Data->selections.begin();
  for( ; siter != this->Data->selections.end(); siter++)
    {
    highlight = dynamic_cast<QHistogramHighlight *>(*siter);
    if(!highlight || !highlight->Bounds.isValid())
      continue;

    if(highlight->Bounds.intersects(area))
      p->fillRect(highlight->Bounds, this->Select);
    }
}

void pqHistogramChart::drawChart(QPainter *p, const QRect &area)
{
  if(!this->Data || !p || !p->isActive() || !area.isValid())
    return;
  if(!this->Bounds.isValid())
    return;

  // Draw in the histogram bars.
  int i = 0;
  bool areaFound = false;
  int total = this->XAxis->getNumberShowing();
  QHistogramHighlight *highlight = 0;
  pqHistogramSelectionList::Iterator siter = this->Data->selections.begin();
  vtkstd::vector<QHistogramItem *>::iterator iter = this->Data->items.begin();
  for( ; iter != this->Data->items.end() && i < total; iter++, i++)
    {
    // Make sure the bounding rectangle is known.
    QHistogramItem *item = *iter;
    if(!item || !item->Bounds.isValid())
      continue;

    // Check to see if the bar needs to be drawn.
    if(!(item->Bounds.left() > area.right() ||
        item->Bounds.right() < area.left()))
      {
      areaFound = true;
      if(!(item->Bounds.top() > area.bottom() ||
          item->Bounds.bottom() < area.top()))
        {
        // First, fill the rectangle. If a bar color scheme is set,
        // get the bar color from the scheme.
        QColor bin = Qt::red;
        if(this->Colors)
          bin = this->Colors->getColor(i, getBinCount());
        p->fillRect(item->Bounds, bin);

        // Draw in the highlighted portion of the bar if it is selected
        // and fill highlighting is being used.
        if(this->Style == pqHistogramChart::Fill)
          {
          for( ; siter != this->Data->selections.end(); siter++)
            {
            highlight = dynamic_cast<QHistogramHighlight *>(*siter);
            if(!highlight)
              continue;
            if(item->Bounds.right() < highlight->Bounds.left())
              break;
            else if(item->Bounds.left() <= highlight->Bounds.right())
              {
              p->fillRect(item->Bounds.intersect(highlight->Bounds),
                  bin.light(170));
              if(item->Bounds.right() <= highlight->Bounds.right())
                break;
              }
            }
          }

        // Draw in the outline last to make sure it is on top.
        if(this->OutlineType == pqHistogramChart::Darker)
          p->setPen(bin.dark());
        else
          p->setPen(Qt::black);
        p->drawRect(item->Bounds.x(), item->Bounds.y(),
          item->Bounds.width() - 1, item->Bounds.height() - 1);

        // If the bar is selected and outline highlighting is used, draw
        // a lighter and thicker outline around the bar.
        if(this->Style == pqHistogramChart::Outline)
          {
          for( ; siter != this->Data->selections.end(); siter++)
            {
            highlight = dynamic_cast<QHistogramHighlight *>(*siter);
            if(!highlight)
              continue;
            if(item->Bounds.right() < highlight->Bounds.left())
              break;
            else if(item->Bounds.left() <= highlight->Bounds.right())
              {
              p->setPen(bin.light(170));
              QRect inter = item->Bounds.intersect(highlight->Bounds);
              inter.setWidth(inter.width() - 1);
              inter.setHeight(inter.height() - 1);
              p->drawRect(inter);
              inter.translate(1, 1);
              inter.setWidth(inter.width() - 2);
              inter.setHeight(inter.height() - 2);
              p->drawRect(inter);
              if(item->Bounds.right() <= highlight->Bounds.right())
                break;
              }
            }
          }
        }
      }
    else if(areaFound)
      break;
    }

  // Draw in the selection outline.
  p->setPen(QColor(60, 90, 135));
  siter = this->Data->selections.begin();
  for( ; siter != this->Data->selections.end(); siter++)
    {
    highlight = dynamic_cast<QHistogramHighlight *>(*siter);
    if(!highlight || !highlight->Bounds.isValid())
      continue;

    if(highlight->Bounds.intersects(area))
      {
      p->drawRect(highlight->Bounds.x(), highlight->Bounds.y(),
          highlight->Bounds.width() - 1, highlight->Bounds.height() - 1);
      }
    }
}

void pqHistogramChart::layoutSelection()
{
  if(this->Data && this->Bounds.isValid())
    {
    QHistogramHighlight *highlight = 0;
    pqHistogramSelectionList::Iterator iter = this->Data->selections.begin();
    for( ; iter != this->Data->selections.end(); iter++)
      {
      highlight = dynamic_cast<QHistogramHighlight *>(*iter);
      if(highlight)
        {
        highlight->Bounds.setTop(this->Bounds.top());
        highlight->Bounds.setBottom(this->Bounds.bottom());
        if(highlight->getType() == pqHistogramSelection::Value)
          {
          highlight->Bounds.setLeft(this->XAxis->getPixelFor(
              highlight->getFirst()));
          highlight->Bounds.setRight(this->XAxis->getPixelFor(
              highlight->getSecond()));
          }
        else
          {
          pqChartValue left = this->XAxis->getMinValue();
          pqChartValue right = left;
          left += this->XAxis->getInterval() * highlight->getFirst();
          right += this->XAxis->getInterval() *
              (highlight->getSecond() + 1);
          highlight->Bounds.setLeft(this->XAxis->getPixelFor(left));
          highlight->Bounds.setRight(this->XAxis->getPixelFor(right));
          }
        }
      }
    }
}

void pqHistogramChart::resetData()
{
  if(this->Data)
    {
    this->Data->clearSelection();
    vtkstd::vector<QHistogramItem *>::iterator iter = this->Data->items.begin();
    for( ; iter != this->Data->items.end(); iter++)
      {
      delete *iter;
      *iter = 0;
      }

    this->Data->items.clear();
    }
}


