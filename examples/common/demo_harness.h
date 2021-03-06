// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <cstdint>
#include <vulkan/vulkan.hpp>

#include "escher/renderer/image_owner.h"
#include "escher/vk/vulkan_context.h"
#include "escher/vk/vulkan_swapchain.h"

#include "vulkan_proc_addrs.h"

class Demo;

// DemoHarness is responsible for initializing Vulkan and its connection to the
// window system, and handling mouse/touch/keyboard input.  Subclasses provide
// platform-specific implementations of this functionality.
class DemoHarness {
 public:
  struct WindowParams {
    std::string window_name;
    uint32_t width = 1024;
    uint32_t height = 1024;
    uint32_t desired_swapchain_image_count = 2;
    bool use_fullscreen = false;
  };

  struct InstanceParams {
    std::vector<std::string> layer_names{"VK_LAYER_LUNARG_standard_validation"};
    std::vector<std::string> extension_names;
  };

  static std::unique_ptr<DemoHarness> New(
      DemoHarness::WindowParams window_params,
      DemoHarness::InstanceParams instance_params);
  virtual ~DemoHarness();

  const std::vector<vk::LayerProperties>& GetInstanceLayers() const {
    return instance_layers_;
  }
  const std::vector<vk::ExtensionProperties>& GetInstanceExtensions() const {
    return instance_extensions_;
  }

  escher::VulkanContext GetVulkanContext();
  escher::VulkanSwapchain GetVulkanSwapchain() { return swapchain_; }

  // Notify the demo that it should stop looping and quit.
  void SetShouldQuit() { should_quit_ = true; }
  bool ShouldQuit() { return should_quit_; }

  virtual void Run(Demo* demo) = 0;
  Demo* GetRunningDemo() { return demo_; }

  // Must be called before harness is destroyed.
  void Shutdown();

 protected:
  // Create via DemoHarness::New().
  DemoHarness(WindowParams window_params, InstanceParams instance_params);

  // Subclasses are responsible for setting this when they start running a Demo,
  // and setting it back to nullptr when they finish running the Demo.
  Demo* demo_ = nullptr;

  vk::Device device() { return device_; }
  vk::Instance instance() { return instance_; }
  vk::SurfaceKHR surface() { return surface_; }
  void set_surface(vk::SurfaceKHR surf) { surface_ = surf; }

 private:
  // For wrapping swapchain images in VkImage.
  // TODO: Find a nicer solution.
  class SwapchainImageOwner : public escher::ImageOwner {
   public:
    explicit SwapchainImageOwner(const escher::VulkanContext& context);
    using escher::ImageOwner::CreateImage;

   private:
    void ReceiveResourceCore(
        std::unique_ptr<escher::ResourceCore> core) override;
  };

  // Called by New() after instantiation is complete, so that virtual functions
  // can be called upon the harness.
  void Init();

  // Called by Init().
  virtual void InitWindowSystem() = 0;
  void CreateInstance(InstanceParams params);
  virtual void CreateWindowAndSurface(const WindowParams& window_params) = 0;
  void CreateDeviceAndQueue();
  void CreateSwapchain(const WindowParams& window_params);

  // Called by Init() via CreateInstance().
  virtual void AppendPlatformSpecificInstanceExtensionNames(
      InstanceParams* params) = 0;

  // Called by Shutdown().
  void DestroySwapchain();
  void DestroyDevice();
  void DestroyInstance();
  virtual void ShutdownWindowSystem() = 0;

  // Redirect to instance method.
  static VkBool32 RedirectDebugReport(VkDebugReportFlagsEXT flags,
                                      VkDebugReportObjectTypeEXT objectType,
                                      uint64_t object,
                                      size_t location,
                                      int32_t messageCode,
                                      const char* pLayerPrefix,
                                      const char* pMessage,
                                      void* pUserData) {
    return reinterpret_cast<DemoHarness*>(pUserData)->HandleDebugReport(
        flags, objectType, object, location, messageCode, pLayerPrefix,
        pMessage);
  }

  VkBool32 HandleDebugReport(VkDebugReportFlagsEXT flags,
                             VkDebugReportObjectTypeEXT objectType,
                             uint64_t object,
                             size_t location,
                             int32_t messageCode,
                             const char* pLayerPrefix,
                             const char* pMessage);

  WindowParams window_params_;
  InstanceParams instance_params_;

  vk::Instance instance_;
  vk::SurfaceKHR surface_;
  vk::PhysicalDevice physical_device_;
  vk::Device device_;
  vk::Queue queue_;
  uint32_t queue_family_index_ = UINT32_MAX;  // initialize to invalid index.
  vk::Queue transfer_queue_;
  uint32_t transfer_queue_family_index_ = UINT32_MAX;  // invalid index.
  escher::VulkanSwapchain swapchain_;

  VkDebugReportCallbackEXT debug_report_callback_;

  InstanceProcAddrs instance_procs_;
  DeviceProcAddrs device_procs_;

  std::unique_ptr<SwapchainImageOwner> swapchain_image_owner_;
  uint32_t swapchain_image_count_ = 0;

  std::vector<vk::LayerProperties> instance_layers_;
  std::vector<vk::ExtensionProperties> instance_extensions_;

  vk::InstanceCreateInfo instance_create_info;

  bool should_quit_ = false;
  bool shutdown_complete_ = false;
};
