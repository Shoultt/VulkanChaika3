#include <vector>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

struct Vertex
{
	float pos[3];
};

glm::mat4 MVP;

static std::vector<VkVertexInputBindingDescription> getVertexBindingDescrip()
{
	std::vector<VkVertexInputBindingDescription> vertexBindinDescrips(1);
	vertexBindinDescrips[0].binding = 0;
	vertexBindinDescrips[0].stride = sizeof(float) * 3;
	vertexBindinDescrips[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return vertexBindinDescrips;
}

static std::vector<VkVertexInputAttributeDescription> getVertexAttributeDescrips()
{
	std::vector<VkVertexInputAttributeDescription> vertexAttributeDescrips(1);
	vertexAttributeDescrips[0].location = 0;
	vertexAttributeDescrips[0].binding = 0;
	vertexAttributeDescrips[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	vertexAttributeDescrips[0].offset = 0;
	return vertexAttributeDescrips;
}
