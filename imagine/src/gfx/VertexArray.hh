#pragma once

class VertexArray
{
public:
	void init(const void *data, size_t size);
	void write(const void *data, size_t size);
	void deinit();
//private:
	VertexArrayRef ref;
};
