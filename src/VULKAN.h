#ifndef CODE_VK_H
#define CODE_VK_H
#include <stdint.h>
#include <stddef.h>

typedef uint32_t VkFlags;
typedef uint32_t VkBool32;
typedef uint64_t VkDeviceSize;
typedef int32_t VkResult;
typedef struct VkInstance_T *VkInstance;
typedef struct VkPhysicalDevice_T *VkPhysicalDevice;
typedef struct VkDevice_T *VkDevice;
typedef struct VkQueue_T *VkQueue;
typedef struct VkCommandBuffer_T *VkCommandBuffer;
typedef uint64_t VkBuffer, VkDeviceMemory, VkShaderModule, VkPipeline,
	VkPipelineLayout, VkDescriptorSetLayout, VkDescriptorPool, VkDescriptorSet,
	VkCommandPool, VkFence, VkSemaphore, VkSampler, VkPipelineCache,
	VkBufferView, VkImageView;

enum
{
	VKS_APPLICATION_INFO = 0,
	VKS_INSTANCE_CREATE_INFO = 1,
	VKS_DEVICE_QUEUE_CREATE_INFO = 2,
	VKS_DEVICE_CREATE_INFO = 3,
	VKS_SUBMIT_INFO = 4,
	VKS_MEMORY_ALLOCATE_INFO = 5,
	VKS_FENCE_CREATE_INFO = 8,
	VKS_BUFFER_CREATE_INFO = 12,
	VKS_SHADER_MODULE_CREATE_INFO = 16,
	VKS_PIPELINE_SHADER_STAGE_CREATE_INFO = 18,
	VKS_COMPUTE_PIPELINE_CREATE_INFO = 29,
	VKS_PIPELINE_LAYOUT_CREATE_INFO = 30,
	VKS_DESCRIPTOR_SET_LAYOUT_CREATE_INFO = 32,
	VKS_DESCRIPTOR_POOL_CREATE_INFO = 33,
	VKS_DESCRIPTOR_SET_ALLOCATE_INFO = 34,
	VKS_WRITE_DESCRIPTOR_SET = 35,
	VKS_COMMAND_POOL_CREATE_INFO = 39,
	VKS_COMMAND_BUFFER_ALLOCATE_INFO = 40,
	VKS_COMMAND_BUFFER_BEGIN_INFO = 42,
	VKS_MEMORY_BARRIER = 46
};

enum
{
	VK_DESCRIPTOR_TYPE_STORAGE_BUFFER = 7,
	VK_SHADER_STAGE_COMPUTE_BIT = 0X20,
	VK_PIPELINE_BIND_POINT_COMPUTE = 1
};

enum
{
	VK_BUFFER_USAGE_TRANSFER_SRC_BIT = 1,
	VK_BUFFER_USAGE_TRANSFER_DST_BIT = 2,
	VK_BUFFER_USAGE_STORAGE_BUFFER_BIT = 0X20
};

enum
{
	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT = 1,
	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT = 2,
	VK_MEMORY_PROPERTY_HOST_COHERENT_BIT = 4,
	VK_MEMORY_PROPERTY_HOST_CACHED_BIT = 8
};

enum
{
	VK_QUEUE_COMPUTE_BIT = 2,
	VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT = 0X800,
	VK_PIPELINE_STAGE_TRANSFER_BIT = 0X1000
};

enum
{
	VK_ACCESS_SHADER_READ_BIT = 0X20,
	VK_ACCESS_SHADER_WRITE_BIT = 0X40,
	VK_ACCESS_TRANSFER_READ_BIT = 0X800,
	VK_ACCESS_TRANSFER_WRITE_BIT = 0X1000
};

enum
{
	VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT = 2,
	VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT = 1
};

typedef struct
{
	int32_t sType;
	const void *pNext;
	const char *pApplicationName;
	uint32_t applicationVersion;
	const char *pEngineName;
	uint32_t engineVersion;
	uint32_t apiVersion;
} VkApplicationInfo;

typedef struct
{
	int32_t sType;
	const void *pNext;
	VkFlags flags;
	const VkApplicationInfo *pApplicationInfo;
	uint32_t enabledLayerCount;
	const char *const *ppEnabledLayerNames;
	uint32_t enabledExtensionCount;
	const char *const *ppEnabledExtensionNames;
} VkInstanceCreateInfo;

typedef struct
{
	uint32_t apiVersion, driverVersion, vendorID, deviceID;
	int32_t deviceType;
	char deviceName[256];
	uint8_t pipelineCacheUUID[16];
	uint8_t rest[1024];
} VkPhysicalDevicePropertiesX;

typedef struct
{
	VkFlags propertyFlags;
	uint32_t heapIndex;
} VkMemoryType;

typedef struct
{
	VkDeviceSize size;
	VkFlags flags;
} VkMemoryHeap;

typedef struct
{
	uint32_t memoryTypeCount;
	VkMemoryType memoryTypes[32];
	uint32_t memoryHeapCount;
	VkMemoryHeap memoryHeaps[16];
} VkPhysicalDeviceMemoryProperties;

typedef struct
{
	VkFlags queueFlags;
	uint32_t queueCount, timestampValidBits;
	uint32_t minImageTransferGranularity[3];
} VkQueueFamilyProperties;

typedef struct
{
	int32_t sType;
	const void *pNext;
	VkFlags flags;
	uint32_t queueFamilyIndex, queueCount;
	const float *pQueuePriorities;
} VkDeviceQueueCreateInfo;

typedef struct
{
	int32_t sType;
	const void *pNext;
	VkFlags flags;
	uint32_t queueCreateInfoCount;
	const VkDeviceQueueCreateInfo *pQueueCreateInfos;
	uint32_t enabledLayerCount;
	const char *const *ppEnabledLayerNames;
	uint32_t enabledExtensionCount;
	const char *const *ppEnabledExtensionNames;
	const void *pEnabledFeatures;
} VkDeviceCreateInfo;

typedef struct
{
	int32_t sType;
	const void *pNext;
	VkFlags flags;
	VkDeviceSize size;
	VkFlags usage;
	int32_t sharingMode;
	uint32_t queueFamilyIndexCount;
	const uint32_t *pQueueFamilyIndices;
} VkBufferCreateInfo;

typedef struct
{
	VkDeviceSize size, alignment;
	uint32_t memoryTypeBits;
} VkMemoryRequirements;

typedef struct
{
	int32_t sType;
	const void *pNext;
	VkDeviceSize allocationSize;
	uint32_t memoryTypeIndex;
} VkMemoryAllocateInfo;

typedef struct
{
	int32_t sType;
	const void *pNext;
	VkFlags flags;
	size_t codeSize;
	const uint32_t *pCode;
} VkShaderModuleCreateInfo;

typedef struct
{
	uint32_t binding;
	int32_t descriptorType;
	uint32_t descriptorCount;
	VkFlags stageFlags;
	const VkSampler *pImmutableSamplers;
} VkDescriptorSetLayoutBinding;

typedef struct
{
	int32_t sType;
	const void *pNext;
	VkFlags flags;
	uint32_t bindingCount;
	const VkDescriptorSetLayoutBinding *pBindings;
} VkDescriptorSetLayoutCreateInfo;

typedef struct
{
	VkFlags stageFlags;
	uint32_t offset, size;
} VkPushConstantRange;

typedef struct
{
	int32_t sType;
	const void *pNext;
	VkFlags flags;
	uint32_t setLayoutCount;
	const VkDescriptorSetLayout *pSetLayouts;
	uint32_t pushConstantRangeCount;
	const VkPushConstantRange *pPushConstantRanges;
} VkPipelineLayoutCreateInfo;

typedef struct
{
	int32_t sType;
	const void *pNext;
	VkFlags flags;
	VkFlags stage;
	VkShaderModule module;
	const char *pName;
	const void *pSpecializationInfo;
} VkPipelineShaderStageCreateInfo;

typedef struct
{
	int32_t sType;
	const void *pNext;
	VkFlags flags;
	VkPipelineShaderStageCreateInfo stage;
	VkPipelineLayout layout;
	VkPipeline basePipelineHandle;
	int32_t basePipelineIndex;
} VkComputePipelineCreateInfo;

typedef struct
{
	int32_t type;
	uint32_t descriptorCount;
} VkDescriptorPoolSize;

typedef struct
{
	int32_t sType;
	const void *pNext;
	VkFlags flags;
	uint32_t maxSets, poolSizeCount;
	const VkDescriptorPoolSize *pPoolSizes;
} VkDescriptorPoolCreateInfo;

typedef struct
{
	int32_t sType;
	const void *pNext;
	VkDescriptorPool descriptorPool;
	uint32_t descriptorSetCount;
	const VkDescriptorSetLayout *pSetLayouts;
} VkDescriptorSetAllocateInfo;

typedef struct
{
	VkBuffer buffer;
	VkDeviceSize offset, range;
} VkDescriptorBufferInfo;

typedef struct
{
	int32_t sType;
	const void *pNext;
	VkDescriptorSet dstSet;
	uint32_t dstBinding, dstArrayElement, descriptorCount;
	int32_t descriptorType;
	const void *pImageInfo;
	const VkDescriptorBufferInfo *pBufferInfo;
	const VkBufferView *pTexelBufferView;
} VkWriteDescriptorSet;

typedef struct
{
	int32_t sType;
	const void *pNext;
	VkFlags flags;
	uint32_t queueFamilyIndex;
} VkCommandPoolCreateInfo;

typedef struct
{
	int32_t sType;
	const void *pNext;
	VkCommandPool commandPool;
	int32_t level;
	uint32_t commandBufferCount;
} VkCommandBufferAllocateInfo;

typedef struct
{
	int32_t sType;
	const void *pNext;
	VkFlags flags;
	const void *pInheritanceInfo;
} VkCommandBufferBeginInfo;

typedef struct
{
	int32_t sType;
	const void *pNext;
	uint32_t waitSemaphoreCount;
	const VkSemaphore *pWaitSemaphores;
	const VkFlags *pWaitDstStageMask;
	uint32_t commandBufferCount;
	const VkCommandBuffer *pCommandBuffers;
	uint32_t signalSemaphoreCount;
	const VkSemaphore *pSignalSemaphores;
} VkSubmitInfo;

typedef struct
{
	int32_t sType;
	const void *pNext;
	VkFlags srcAccessMask, dstAccessMask;
} VkMemoryBarrier;

typedef struct
{
	VkDeviceSize srcOffset, dstOffset, size;
} VkBufferCopy;

#endif
