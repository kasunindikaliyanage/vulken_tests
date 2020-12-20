#pragma once

struct QueueFamilyIndicies{
	int graphics_family = -1;

	//check if queue families are valid
	bool is_valid()
	{
		return graphics_family >= 0;
	}
};