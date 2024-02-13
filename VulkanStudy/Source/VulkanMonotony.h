#pragma once

#define VK_USE_PLATFORM_WIN_32
#include <vulkan/vulkan.h>
#include "source/initWindow.h"
#include "Shaders/shaderReader.h"
#include "Models/vertexInfo.h"

#define VMA_IMPLEMENTATION
#include <VMA/vk_mem_alloc.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

VkInstance instance;
VkPhysicalDevice physDevice;
VkDevice device;
VkQueue queue;
VkSurfaceKHR surface;
VkSwapchainKHR swapchain;
VkRenderPass renPass;
VkDescriptorSetLayout descriptSetLayout;
VkPipelineLayout pipeLayout;
VkPipeline graphPipeline;
VkDescriptorPool descriptPool;
VkDescriptorSet descriptorSet;
VkCommandPool cmdPool;
VkCommandBuffer cmdBuffer;

std::vector<VkImage> swapImages;
std::vector<VkImageView> swapImageViews;
std::vector<VkFramebuffer> frameBuffers;

VkShaderModule vsModule;
VkShaderModule fsModule;

VkSemaphore waitSemaphore;
VkSemaphore signalSemaphore;
VkFence fence;

VmaAllocator allocator;

VkBuffer vertexBuffer;
VmaAllocation vertexBufferAlloc;
VkBuffer indexBuffer;
VmaAllocation indexBufferAlloc;
VkBuffer uniformBuffer;
VmaAllocation uniformBufferAlloc;
VmaAllocationInfo uniformBufferAllocInfo = {};
VkImage textureImage;
VmaAllocation textureImageAlloc;
VkImageView textureImageView;
VkSampler textureSampler;
VkImage depthImage;
VmaAllocation depthImageAlloc;
VkImageView depthImageView;

#ifdef NDEBUG
bool enableValidationLayers = false;
#else
bool enableValidationLayers = true;
#endif

VkPhysicalDeviceProperties physDevProps{};

std::vector<const char*> layers{ "VK_LAYER_KHRONOS_validation" };
std::vector<const char*> ExtsIns{ "VK_KHR_surface", "VK_KHR_win32_surface" };
std::vector<const char*> ExtDev{ "VK_KHR_swapchain", "VK_KHR_dynamic_rendering" };

std::vector<char> vsCode = shaderReader("Shaders/vert.spv");
std::vector<char> fsCode = shaderReader("Shaders/frag.spv");

VkDeviceSize offsets[] = { 0 };

textureReader textureRead;

void createShaderModeles(const std::vector<char> &code, VkShaderModule* sModule)
{
	VkShaderModuleCreateInfo smci = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	smci.codeSize = code.size();
	smci.pCode = (uint32_t*)code.data();

	vkCreateShaderModule(device, &smci, nullptr, sModule);
}

void createAllocator()
{
	VmaAllocatorCreateInfo allocatorCI = {};
	allocatorCI.device = device;
	allocatorCI.instance = instance;
	allocatorCI.physicalDevice = physDevice;

	vmaCreateAllocator(&allocatorCI, &allocator);
}

VkCommandBuffer startTimeCommands()
{
	VkCommandBufferAllocateInfo cmdBufferAI;
	cmdBufferAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufferAI.pNext = nullptr;
	cmdBufferAI.commandPool = cmdPool;
	cmdBufferAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufferAI.commandBufferCount = 1;
	vkAllocateCommandBuffers(device, &cmdBufferAI, &cmdBuffer);

	VkCommandBufferBeginInfo cmdBI = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	cmdBI.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmdBuffer, &cmdBI);

	return cmdBuffer;
}

void endTimeCommands()
{
	vkEndCommandBuffer(cmdBuffer);

	VkSubmitInfo SI = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	SI.commandBufferCount = 1;
	SI.pCommandBuffers = &cmdBuffer;

	vkQueueSubmit(queue, 1, &SI, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);
}

void createBuffer(VkBuffer& buffer, VkDeviceSize bufferSize, void* data, VkBufferUsageFlags flag, VmaAllocation& bufferAlloc)
{
	VkBufferCreateInfo bufferCI = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferCI.size = bufferSize;
	bufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo bAllocCI = {};
	bAllocCI.usage = VMA_MEMORY_USAGE_AUTO;
	bAllocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VkBuffer stagingBuffer = VK_NULL_HANDLE;
	VmaAllocation stagingAllocation = VK_NULL_HANDLE;
	VmaAllocationInfo stagingBufferAllocInfo{};
	vmaCreateBuffer(allocator, &bufferCI, &bAllocCI, &stagingBuffer, &stagingAllocation, &stagingBufferAllocInfo);

	memcpy(stagingBufferAllocInfo.pMappedData, data, bufferCI.size);

	bufferCI.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | flag;
	bAllocCI.flags = 0;

	vmaCreateBuffer(allocator, &bufferCI, &bAllocCI, &buffer, &bufferAlloc, nullptr);

	VkCommandBuffer cmdBuffer = 
	startTimeCommands();
	{
		VkBufferCopy region = {};
		region.size = bufferCI.size;

		vkCmdCopyBuffer(cmdBuffer, stagingBuffer, buffer, 1, &region);
	}
	endTimeCommands();

	vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
	stagingBuffer = VK_NULL_HANDLE;
	stagingAllocation = VK_NULL_HANDLE;
}

void createUnibuffer(VkBuffer& uniformBuffer, VkDeviceSize bufferSize, VmaAllocation& uniformBufferAlloc, VmaAllocationInfo& allocInfo)
{
	VkBufferCreateInfo bufferCI = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferCI.size = bufferSize;
	bufferCI.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocCI = {};
	allocCI.usage = VMA_MEMORY_USAGE_AUTO;
	allocCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

	vmaCreateBuffer(allocator, &bufferCI, &allocCI, &uniformBuffer, &uniformBufferAlloc, &allocInfo);
}

void uploadTexture(const char* filename, VmaAllocation& imageAlloc)
{
	textureRead.load(filename);
	uint32_t imageWidth = textureRead.getTextureWidth();
	uint32_t imageHeight = textureRead.getTextureHeight();

	VkBufferCreateInfo bufferCI = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferCI.size = textureRead.getTextureSize();
	bufferCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocCI = {};
	allocCI.usage = VMA_MEMORY_USAGE_AUTO;
	allocCI.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

	VkBuffer stagingBuffer = VK_NULL_HANDLE;
	VmaAllocation stagingAllocation = VK_NULL_HANDLE;
	VmaAllocationInfo stagingAllocInfo = {};
	vmaCreateBuffer(allocator, &bufferCI, &allocCI, &stagingBuffer, &stagingAllocation, &stagingAllocInfo);
	memcpy(stagingAllocInfo.pMappedData, textureRead.getPicture(), bufferCI.size);

	allocCI.flags = 0;
	bufferCI.usage = 0;
	VkImageCreateInfo imageCI = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageCI.imageType = VK_IMAGE_TYPE_2D;
	imageCI.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageCI.extent = {imageWidth, imageHeight, 1};
	imageCI.mipLevels = 1;
	imageCI.arrayLayers = 1;
	imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	vmaCreateImage(allocator, &imageCI, &allocCI, &textureImage, &imageAlloc, nullptr);

	VkCommandBuffer cmdBuffer =
	startTimeCommands();
	{
		VkImageMemoryBarrier ImageBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		ImageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		ImageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ImageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		ImageBarrier.image = textureImage;
		ImageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ImageBarrier.subresourceRange.levelCount = 1;
		ImageBarrier.subresourceRange.layerCount = 1;
		vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &ImageBarrier);

		VkBufferImageCopy region{};
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { imageWidth, imageHeight, 1 };
		vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer, textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		ImageBarrier.image = textureImage;
		ImageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		ImageBarrier.dstAccessMask = VK_ACCESS_NONE;
		ImageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		ImageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &ImageBarrier);
	}
	endTimeCommands(); 

	textureRead.destroy();
	vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
	stagingBuffer = VK_NULL_HANDLE;
	stagingAllocation = VK_NULL_HANDLE;
}

VkImageView createImageViews(VkImage& image, VkFormat format, VkImageAspectFlags imageAspectFlag)
{
	VkImageViewCreateInfo imageViewCI = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	imageViewCI.image = image;
	imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCI.format = format;
	imageViewCI.subresourceRange.aspectMask = imageAspectFlag;
	imageViewCI.subresourceRange.baseMipLevel = 0;
	imageViewCI.subresourceRange.levelCount = 1;
	imageViewCI.subresourceRange.baseArrayLayer = 0;
	imageViewCI.subresourceRange.layerCount = 1;

	VkImageView imageView;
	vkCreateImageView(device, &imageViewCI, nullptr, &imageView);

	return imageView;
}

void createTextureImageView()
{
	textureImageView = createImageViews(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void createSampler()
{
	VkSamplerCreateInfo samplerCI = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	samplerCI.magFilter = VK_FILTER_LINEAR;
	samplerCI.minFilter = VK_FILTER_LINEAR;
	samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCI.anisotropyEnable = VK_TRUE;
	samplerCI.maxAnisotropy = physDevProps.limits.maxSamplerAnisotropy;
	samplerCI.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerCI.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

	vkCreateSampler(device, &samplerCI, nullptr, &textureSampler);
}

void createDepthImage()
{
	VkImageCreateInfo depthImageCI = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	depthImageCI.imageType = VK_IMAGE_TYPE_2D;
	depthImageCI.format = VK_FORMAT_D24_UNORM_S8_UINT;
	depthImageCI.extent = {Wwidth, Wheight, 1};
	depthImageCI.mipLevels = 1;
	depthImageCI.arrayLayers = 1;
	depthImageCI.samples = VK_SAMPLE_COUNT_1_BIT;
	depthImageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
	depthImageCI.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	depthImageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	depthImageCI.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

	VmaAllocationCreateInfo depthAllocCI = {};
	depthAllocCI.usage = VMA_MEMORY_USAGE_AUTO;

	vmaCreateImage(allocator, &depthImageCI, &depthAllocCI, &depthImage, &depthImageAlloc, nullptr);
}

void createDepthImageView()
{
	depthImageView = createImageViews(depthImage, VK_FORMAT_D24_UNORM_S8_UINT, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void createDepthImageLayout()
{
	VkCommandBuffer cmdBuffer =
	startTimeCommands();
	{
		VkImageMemoryBarrier depthImageMemBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		depthImageMemBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		depthImageMemBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthImageMemBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthImageMemBarrier.image = depthImage;
		depthImageMemBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		depthImageMemBarrier.subresourceRange.levelCount = 1;
		depthImageMemBarrier.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &depthImageMemBarrier);
	}
	endTimeCommands();
}