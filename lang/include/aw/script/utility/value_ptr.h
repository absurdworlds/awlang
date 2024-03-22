#ifndef aw_utility_value_ptr_h
#define aw_utility_value_ptr_h

#include <memory>

namespace aw::script {
template<typename T>
struct value_ptr : std::unique_ptr<T> {
	using base = std::unique_ptr<T>;

	value_ptr() = default;

	value_ptr(base&& other) noexcept
		: base(std::move(other))
	{
	}

	value_ptr(value_ptr&& other) noexcept
		: base(std::move(other))
	{
	}

	value_ptr(const base& other)
		: base(other ? new T(*other) : nullptr)
	{
	}

	template<typename U>
	value_ptr(U&& value)
		: base(new T(std::move(value)))
	{
	}

	value_ptr(const value_ptr& other)
		: base(other ? new T(*other) : nullptr)
	{
	}

	value_ptr& operator=(value_ptr&& other) noexcept
	{
		base::operator=(std::move(other));
		return *this;
	}

	value_ptr& operator=(const value_ptr& other)
	{
		auto& self = *this;
		if (self && other)
			*this = *other;
		else if (self)
			self.reset(nullptr);
		else if (other)
			self.reset(new T(*other));
		return *this;
	}

	friend auto clone(const value_ptr<T>& ptr) -> value_ptr<T>
	{
		return ptr;
	}
};



} // namespace aw::script

#endif // aw_utility_value_ptr_h
