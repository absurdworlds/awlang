/*
 * Copyright (C) 2016-2022 Hudd <haddayn@gmail.com>
 *
 * License LGPLv3 or later:
 * GNU Lesser GPL version 3 <http://gnu.org/licenses/lgpl-3.0.html>
 * This is free software: you are free to change and redistribute it.
 * There is NO WARRANTY, to the extent permitted by law.
 */
#ifndef aw_script_source_buffer_h
#define aw_script_source_buffer_h
#include <aw/io/file.h>
#include <aw/io/read_file.h>
#include <aw/types/char_buffer.h>
#include <string>
namespace aw::script {
/*!
 * Wraps access to source files
 */
class source_buffer {
public:
	explicit source_buffer(std::string_view view, unsigned id = 0, fs::path path = {})
		: buffer(view)
		, path(std::move(path))
		, id(id)
	{

	}

	// TODO: Add support for mmap_file
	explicit source_buffer(aw::io::read_file<aw::io::file>& file, unsigned id = 0)
		: source_buffer(id, file.path(), read_file(file))
	{
	}

	source_buffer(source_buffer&& other) noexcept = default;

	source_buffer& operator=(source_buffer&& other) noexcept = default;

	unsigned file_id() const
	{
		return id;
	}

	char const* begin() const
	{
		return buffer.begin();
	}

	char const* end() const
	{
		return buffer.end();
	}

	size_t size() const
	{
		return buffer.view_size();
	}

private:
	struct buffer_data {
		char* ptr = nullptr;
		size_t size = 0;
		size_t view_length = size;
	};

	explicit source_buffer(unsigned id, fs::path path, buffer_data data)
		: buffer(data.ptr, data.size, data.view_length)
		, path(std::move(path))
		, id(id)
	{
	}

	static auto read_file(aw::io::read_file<aw::io::file>& file) -> buffer_data
	{
		if (!file.is_open())
			return {};

		auto length = file.size();

		std::unique_ptr<char[]> buffer(new char[length + 1]);

		auto res = file.read(buffer.get(), length);

		if (res == -1)
			buffer.reset();

		// Sentinel
		buffer[length] = 0;

		return buffer_data{
			.ptr = buffer.release(),
			.size = length + 1,
			.view_length = length,
		};
	}

	char_buffer buffer;
	fs::path path;
	unsigned id = 0;
};
} // namespace aw::script
#endif//aw_script_source_buffer_h
