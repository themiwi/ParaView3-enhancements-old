/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "ClientTreeView.h"

#include <vtkAbstractArray.h>
#include <vtkAnnotationLink.h>
#include <vtkCommand.h>
#include <vtkConvertSelection.h>
#include <vtkDataObjectTypes.h>
#include <vtkDataRepresentation.h>
#include <vtkDataSetAttributes.h>
#include <vtkGraph.h>
#include <vtkIdTypeArray.h>
#include <vtkIntArray.h>
#include <vtkPVDataInformation.h>
#include <vtkQtTreeView.h>
#include <vtkScalarsToColors.h>
#include <vtkSelection.h>
#include <vtkSelectionNode.h>
#include <vtkSmartPointer.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMSelectionDeliveryRepresentationProxy.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMViewProxy.h>
#include <vtkTree.h>
#include <vtkVariantArray.h>
#include <vtkViewTheme.h>

#include <pqDataRepresentation.h>
#include <pqOutputPort.h>
#include <pqPipelineSource.h>
#include <pqRepresentation.h>
#include <pqSelectionManager.h>
#include <pqServer.h>

#include <QPointer>
#include <QVBoxLayout>
#include <QWidget>

////////////////////////////////////////////////////////////////////////////////////
// ClientTreeView::command

class ClientTreeView::command : public vtkCommand
{
public:
  command(ClientTreeView& view) : Target(view) { }
  virtual void Execute(vtkObject*, unsigned long, void*)
  {
    Target.selectionChanged();
  }
  ClientTreeView& Target;
};

////////////////////////////////////////////////////////////////////////////////////
// ClientTreeView::implementation

class ClientTreeView::implementation
{
public:
  implementation()
  {
    this->Widget = new QWidget();
    this->View = vtkSmartPointer<vtkQtTreeView>::New();
    QVBoxLayout *layout = new QVBoxLayout(this->Widget);
    layout->addWidget(this->View->GetWidget());
    layout->setContentsMargins(0,0,0,0);
    this->AttributeType = -1;
    this->LastSelectionMTime = 0;

    this->Theme.TakeReference(vtkViewTheme::CreateNeonTheme());
    this->View->ApplyViewTheme(this->Theme);
  }

  ~implementation()
  {
    this->View->RemoveAllRepresentations();
    if(this->Widget)
      delete this->Widget;
  }

  unsigned long LastSelectionMTime;
  int AttributeType;
  vtkSmartPointer<vtkViewTheme> Theme;
  vtkSmartPointer<vtkQtTreeView> View;
  QPointer<QWidget> Widget;
};

////////////////////////////////////////////////////////////////////////////////////
// ClientTreeView

ClientTreeView::ClientTreeView(
    const QString& viewmoduletype, 
    const QString& group, 
    const QString& name, 
    vtkSMViewProxy* viewmodule, 
    pqServer* server, 
    QObject* p) :
  pqSingleInputView(viewmoduletype, group, name, viewmodule, server, p),
  Implementation(new implementation()),
  Command(new command(*this))
{
  this->Implementation->View->AddObserver(
    vtkCommand::SelectionChangedEvent, this->Command);
}

ClientTreeView::~ClientTreeView()
{
  delete this->Implementation;
  this->Command->Delete();
}

vtkView* ClientTreeView::getClientSideView() const
{
  return this->Implementation->View;
}

QWidget* ClientTreeView::getWidget()
{
  return this->Implementation->Widget;
}

void ClientTreeView::selectionChanged()
{
  // Get the representaion's source
  pqDataRepresentation* pqRepr =
    qobject_cast<pqDataRepresentation*>(this->visibleRepresentation());
  if (!pqRepr)
    {
    return;
    }
  pqOutputPort* opPort = pqRepr->getOutputPortFromInput();
  vtkSMSourceProxy* repSource = vtkSMSourceProxy::SafeDownCast(
    opPort->getSource()->getProxy());

  // Fill the selection source with the selection from the view
  vtkSelection* sel = this->Implementation->View->GetRepresentation()->
    GetAnnotationLink()->GetCurrentSelection();
  vtkSMSourceProxy* selectionSource = pqSelectionManager::createSelectionSource(
    sel, repSource->GetConnectionID());

  // Set the selection on the representation's source
  repSource->SetSelectionInput(opPort->getPortNumber(),
    selectionSource, 0);
  selectionSource->Delete();

  // Mark the annotation link as modified so it will be updated
  if (this->getAnnotationLink())
    {
    this->getAnnotationLink()->MarkModified(0);
    }

  this->Implementation->LastSelectionMTime = repSource->GetSelectionInput(0)->GetMTime();
}

bool ClientTreeView::canDisplay(pqOutputPort* output_port) const
{
  if(!output_port)
    return false;

  pqPipelineSource* const source = output_port->getSource();
  if(!source)
    return false;

  if(this->getServer()->GetConnectionID() != source->getServer()->GetConnectionID())
    return false;

  vtkSMSourceProxy* source_proxy =
    vtkSMSourceProxy::SafeDownCast(source->getProxy());
  if (!source_proxy ||
     source_proxy->GetOutputPortsCreated() == 0)
    {
    return false;
    }

  const char* name = output_port->getDataClassName();
  int type = vtkDataObjectTypes::GetTypeIdFromClassName(name);
  switch(type)
    {
    case VTK_DIRECTED_ACYCLIC_GRAPH:
    case VTK_DIRECTED_GRAPH:
    case VTK_GRAPH:
    case VTK_TREE:
    case VTK_UNDIRECTED_GRAPH:
    case VTK_TABLE:
      return true;
    }

  return false;
}

void ClientTreeView::updateRepresentation(pqRepresentation* repr)
{
/*
  vtkSMClientDeliveryRepresentationProxy* const proxy = repr ? 
    vtkSMClientDeliveryRepresentationProxy::SafeDownCast(repr->getProxy()) : NULL;
  proxy->Update(vtkSMViewProxy::SafeDownCast(this->getProxy()));  

  // Add the representation to the view
  this->Implementation->View->SetRepresentationFromInputConnection(proxy->GetOutputPort());
  */
}

void ClientTreeView::showRepresentation(pqRepresentation* pqRepr)
{
  vtkSMClientDeliveryRepresentationProxy* const proxy = pqRepr ? 
    vtkSMClientDeliveryRepresentationProxy::SafeDownCast(pqRepr->getProxy()) : NULL;
  vtkDataRepresentation* rep = this->Implementation->View->SetRepresentationFromInputConnection(proxy->GetOutputPort());
  rep->SetSelectionType(vtkSelectionNode::PEDIGREEIDS);
  // If we have an associated annotation link proxy, set the client side
  // object as the annotation link on the representation.
  if (this->getAnnotationLink())
    {
    vtkAnnotationLink* link = static_cast<vtkAnnotationLink*>(this->getAnnotationLink()->GetClientSideObject());
    rep->SetAnnotationLink(link);
    }

  //rep->Update();
  this->Implementation->View->Update();
}

void ClientTreeView::hideRepresentation(pqRepresentation* repr)
{
  //this->Implementation->View->RemoveAllRepresentations();
  vtkSMClientDeliveryRepresentationProxy* const proxy = vtkSMClientDeliveryRepresentationProxy::SafeDownCast(repr->getProxy());
  this->Implementation->View->RemoveRepresentation(proxy->GetOutputPort());
  this->Implementation->View->Update();
}

void ClientTreeView::renderInternal()
{
  pqRepresentation* representation = this->visibleRepresentation();
  vtkSMSelectionDeliveryRepresentationProxy* const proxy = representation?
    vtkSMSelectionDeliveryRepresentationProxy::SafeDownCast(representation->getProxy()) : NULL;
  if(!proxy)
    {
    return;
    }

  proxy->Update();

  // Make sure the view type is set (either column or tree)
  this->Implementation->View->SetUseColumnView(
    vtkSMPropertyHelper(this->getProxy(),"UseColumnView").GetAsInt());

  if(this->Implementation->View->GetRepresentation())
    {
    pqDataRepresentation* pqRepr =
      qobject_cast<pqDataRepresentation*>(this->visibleRepresentation());
    pqOutputPort* opPort = pqRepr->getOutputPortFromInput();
    vtkSMSourceProxy* repSource = vtkSMSourceProxy::SafeDownCast(
      opPort->getSource()->getProxy());

    proxy->GetSelectionRepresentation()->Update();
    vtkSelection* sel = vtkSelection::SafeDownCast(
      proxy->GetSelectionRepresentation()->GetOutput());

    // Only use the source proxy's selection if we're not using vtkAnnotationLink directly
    if(!this->getAnnotationLink() && 
      repSource->GetSelectionInput(0) &&
      repSource->GetSelectionInput(0)->GetMTime() > this->Implementation->LastSelectionMTime)
      {
      this->Implementation->LastSelectionMTime = repSource->GetSelectionInput(0)->GetMTime();
      this->Implementation->View->GetRepresentation()->GetAnnotationLink()->SetCurrentSelection(sel);
      this->Implementation->View->GetRepresentation()->Update();
      }
    }

  // Set parameters on the view
  this->Implementation->View->SetShowHeaders(
    vtkSMPropertyHelper(this->getProxy(),"ShowHeaders").GetAsInt());
  this->Implementation->View->SetShowRootNode(
    vtkSMPropertyHelper(this->getProxy(),"ShowRootNode").GetAsInt());
  this->Implementation->View->SetAlternatingRowColors(
    vtkSMPropertyHelper(this->getProxy(),"AlternatingRowColors").GetAsInt());
  this->Implementation->View->SetEnableDragDrop(
    vtkSMPropertyHelper(this->getProxy(),"EnableDragDrop").GetAsInt());

  if(vtkSMPropertyHelper(proxy, "ColorByArray").GetAsInt())
    {
    this->Implementation->View->SetColorByArray(true);

    this->Implementation->View->SetColorArrayName(
      vtkSMPropertyHelper(proxy, "ColorArray").GetAsString());

    vtkSMProxyProperty* lutProp = vtkSMProxyProperty::SafeDownCast(proxy->GetProperty("LookupTable"));
    vtkSmartPointer<vtkViewTheme> theme = vtkSmartPointer<vtkViewTheme>::New();
    if (lutProp->GetNumberOfProxies() > 0)
      {
      vtkScalarsToColors* lut = vtkScalarsToColors::SafeDownCast(
        lutProp->GetProxy(0)->GetClientSideObject());
      theme->SetPointLookupTable(lut);
      }
    theme->SetScalePointLookupTable(vtkSMPropertyHelper(proxy, "ScaleLookupTable").GetAsInt());
    this->Implementation->View->ApplyViewTheme(theme);
    }

  this->Implementation->View->Update();

  // Note: You have to do an update first the View has to have a 
  // populated model in order for HideColumn to work.
  // FIXME: This will only hide one column right now
  this->Implementation->View->HideColumn(
    vtkSMPropertyHelper(this->getProxy(),"HideColumn").GetAsInt());

  if(vtkSMPropertyHelper(this->getProxy(), "HideAllButFirstColumn").GetAsInt())
    {
    this->Implementation->View->HideAllButFirstColumn();
    }
}

