import re

from libvtkPVServerCommonPython import *
from libvtkPVServerManagerPython import *

class pyProxy:
    """
    Proxy wrapper. Makes it easier to set properties on a proxy.
    Instead of:
     proxy.GetProperty("Foo").SetElement(0, 1)
     proxy.GetProperty("Foo").SetElement(0, 2)
    you can do:
     proxy.SetFoo(1,2)
    For proxy properties, you can use AddTo so instead of:
     proxy.GetProoperty("Bar").AddProxy(foo)
    you can do:
     proxy.AddToBar(foo)
    All other methods are passed through to the SMProxy.
    """
    def __init__(self, proxy):
        "Constructor. Assigns proxy to self.SMProxy"
        self.SMProxy = proxy

    def AddToProperty(self, *args):
        """Generic method for adding a proxy to a proxy property.
        Should not be directly called"""
        if not self.__LastAttrName:
            self.__LastAttrName = None
            raise "Cannot find property name"
            return
        property = self.SMProxy.GetProperty(self.__LastAttrName)
        if not property:
            self.__LastAttrName = None
            print "Property %s not found. Cannot set" % self.__LastAttrName
            return
        if property.IsA("vtkSMProxyProperty"):
            for proxy in args:
                try:
                    getattr(proxy, "SMProxy")
                except AttributeError:
                    property.AddProxy(proxy)
                else:
                    property.AddProxy(proxy.SMProxy)
                    
        else:
            raise "AddTo works only with proxy properties"
        self.__LastAttrName = None

    def SetProperty(self, *args):
        """Generic method for setting the value of a property.
        Should not be directly called"""
        if not self.__LastAttrName:
            self.__LastAttrName = None
            raise "Cannot find property name"
            return
        property = self.SMProxy.GetProperty(self.__LastAttrName)
        if not property:
            self.__LastAttrName = None
            print "Property %s not found. Cannot set" % self.__LastAttrName
            return
        if property.IsA("vtkSMProxyProperty"):
            for i in range(len(args)):
                try:
                    getattr(args[i], "SMProxy")
                except AttributeError:
                    property.SetProxy(i, args[i])
                else:
                    property.SetProxy(i, args[i].SMProxy)
        else:
            for i in range(len(args)):
                property.SetElement(i, args[i])
        self.__LastAttrName = None

    def CreateDisplayProxy(self):
        "Overload RenderModules CreateDisplayProxy() to return a pyProxy"
        return pyProxy(self.SMProxy.CreateDisplayProxy())
    
    def __getattr__(self, name):
        """With the exception of a few overloaded methods,
        returns the SMProxy method"""
        if re.compile("^Set").match(name):
            self.__LastAttrName = name[3:]
            return self.SetProperty
        if re.compile("^AddTo").match(name):
            self.__LastAttrName = name[5:]
            return self.AddToProperty
        return getattr(self.SMProxy, name)
        
class pyProxyManager:
    "Proxy manager wrapper"

    def __init__(self):
        """Constructor. Assigned self.SMProxyManager to
        vtkSMObject.GetPropertyManager(). Make sure to initialize
        server manager before creating a pyProxyManager"""
        self.SMProxyManager = vtkSMObject.GetProxyManager()

    def RegisterProxy(self, group, name, proxy):
        """Registers a proxy (either SMProxy or pyProxy) with the
        server manager"""
        try:
            getattr(proxy, "SMProxy")
        except:
            self.SMProxyManager.RegisterProxy(group, name, proxy)
        else:
            self.SMProxyManager.RegisterProxy(group, name, proxy.SMProxy)

    def NewProxy(self, group, name):
        """Creates a new proxy of given group and name and returns a pyProxy
        wrapper"""
        if not self.SMProxyManager:
            return None
        proxy = self.SMProxyManager.NewProxy(group, name)
        if not proxy:
            return None
        return pyProxy(proxy)

    def GetProxy(self, group, name):
        """Returns a pyProxy wrapper for a proxy"""
        if not self.SMProxyManager:
            return None
        proxy = self.SMProxyManager.GetProxy(group, name)
        if not proxy:
            return None
        return pyProxy(proxy)

    def __getattr__(self, name):
        """Returns attribute from the SMProxyManager"""
        return getattr(self.SMProxyManager, name)
