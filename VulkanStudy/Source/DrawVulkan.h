#include "Source/initVulkan.h"
#include "Control.h"

void updateMVP()
{
	float currentFrame = glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
	
	glm::mat4 model = glm::translate(glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(3.0f, 3.0f, 3.0f)), glm::radians(10.0f), glm::vec3(0.0f, 0.0f, 1.0f)), glm::vec3(0.0f, 0.0, 0.0f));
	glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	glm::mat4 projection = glm::perspective(glm::radians(fov), (float)Wwidth / (float)Wheight, 0.1f, 1000.0f);
	projection[1][1] *= -1;
	MVP = projection * view * model;

	memcpy(uniformBufferAllocInfo.pMappedData, &MVP, sizeof(MVP));
}

void recordDrawCmdBuffer(uint32_t imageIndex)
{
	const VkClearColorValue colorValue = { 0.2f, 0.07f, 0.48f, 1.0f };
	const VkClearDepthStencilValue depthStencilValue = { 1.0f, 0 };
	const VkRenderingAttachmentInfo colorAttachment{ .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
	.imageView = swapImageViews[imageIndex],
	.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
	.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
	.clearValue = colorValue };
	const VkRenderingAttachmentInfo depthAttachment{ .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
	.imageView = depthImageView,
	.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
	.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
	.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
	.clearValue = depthStencilValue.depth };
	const VkRenderingInfo renderingInfo{ .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
	.renderArea = {{0, 0}, {Wwidth, Wheight}},
	.layerCount = 1,
	.colorAttachmentCount = 1,
	.pColorAttachments = &colorAttachment,
	.pDepthAttachment = &depthAttachment };
	VkCommandBufferBeginInfo beginInfo{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	vkBeginCommandBuffer(cmdBuffer, &beginInfo);
	const VkImageMemoryBarrier imageMemoryBarrierColor{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
	.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	.image = swapImages[imageIndex],
	.subresourceRange{
	.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	.baseMipLevel = 0,
	.levelCount = 1,
	.baseArrayLayer = 0,
	.layerCount = 1 } };
	vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrierColor);
	
	vkCmdBeginRendering(cmdBuffer, &renderingInfo);
	{
		vkCmdBindIndexBuffer(cmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer, offsets);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeLayout, 0, 1, &descriptorSet, 0, nullptr);
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphPipeline);
		vkCmdDrawIndexed(cmdBuffer, modelRead.getIndicesCount(), 1, 0, 0, 0);
	}
	vkCmdEndRendering(cmdBuffer);

	const VkImageMemoryBarrier imageMemoryBarrierPresent{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
	.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	.image = swapImages[imageIndex],
	.subresourceRange{
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = 1 } };
	vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrierPresent);
	vkEndCommandBuffer(cmdBuffer);
}
void presentFrame()
{
	vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
	vkResetFences(device, 1, &fence);

	uint32_t imageIndex;
	vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, waitSemaphore, VK_NULL_HANDLE, &imageIndex);

	vkResetCommandBuffer(cmdBuffer, 0);
	recordDrawCmdBuffer(imageIndex);

	VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	const VkSubmitInfo submitInfo{ .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	.waitSemaphoreCount = 1,
	.pWaitSemaphores = &waitSemaphore,
	.pWaitDstStageMask = &dstStageMask,
	.commandBufferCount = 1,
	.pCommandBuffers = &cmdBuffer,
	.signalSemaphoreCount = 1,
	.pSignalSemaphores = &signalSemaphore,
	};
	vkQueueSubmit(queue, 1, &submitInfo, fence);

	const VkPresentInfoKHR presentInfo{ .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
	.waitSemaphoreCount = 1,
	.pWaitSemaphores = &signalSemaphore,
	.swapchainCount = 1,
	.pSwapchains = &swapchain,
	.pImageIndices = &imageIndex };
	vkQueuePresentKHR(queue, &presentInfo);
}

void loop()
{
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		control();
		updateMVP();
		presentFrame();
	}
}