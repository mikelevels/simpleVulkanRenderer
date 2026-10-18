// frame.cpp: building a Frame.
#include "frame.h"
#include "image.h"

Frame::Frame(vk::Image image, vk::Device logicalDevice, vk::Format format,
	DeletionQueue& deletionQueue) : image(image) {
	imageView = create_image_view(logicalDevice, image, format);
	// Capture copies of the two handles. The author converts the view to the
	// C type VkImageView first; vk:: handles copy just as cheaply (they hold
	// one pointer-sized value), so there's no need.
	vk::ImageView view = imageView;
	deletionQueue.push([logicalDevice, view] {
		logicalDevice.destroyImageView(view);
	});
}
