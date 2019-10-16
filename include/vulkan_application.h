#ifndef VULKAN_APPLICATION_H
#define VULKAN_APPLICATION_H

#include <QtGui/QGuiApplication>
#include <QtGui/QVulkanInstance>
#include <QtGui/QVulkanWindow>
#include <QtGui/QVulkanWindowRenderer>

class VulkanRenderer : public QVulkanWindowRenderer
{
  public:
    VulkanRenderer(QVulkanWindow* window) : window_(window) {}
  private:
    QVulkanWindow* window_;
};

class VulkanWindow : public QVulkanWindow
{
  public: 
    VulkanWindow() {}
    ~VulkanWindow() {}
    QVulkanWindowRenderer* createRenderer() override {}
};

class VulkanApplication : public QGuiApplication
{
  public:
    VulkanApplication(int& argc, char**& argv) : QGuiApplication(argc, argv)
    {
        if (!vulkan_instance_.create())
        {
            qFatal("Failed to create vulkan instance: %d", vulkan_instance_.errorCode());
        }
        vulkan_window_.setVulkanInstance(&vulkan_instance_);
        vulkan_window_.showMaximized();
    }
  private:
    QVulkanInstance vulkan_instance_;
    VulkanWindow vulkan_window_;
};

#endif  // VULKAN_APPLICATION_H