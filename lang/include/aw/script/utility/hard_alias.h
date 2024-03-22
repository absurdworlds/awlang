#ifndef aw_utility_variant_wrapper_h
#define aw_utility_variant_wrapper_h

namespace aw {

// Make a class that is a T for all intents and purposes
// but with deleted copy constructor/assignment operator
// It is needed due to clang's shenanigans, it attempts to
// generate copy constructors for std::vector<unique_ptr<T>>
// and fails with errors
template<typename T>
struct noncopyable : T {
	noncopyable() = default;

	~noncopyable() = default;

	noncopyable(const noncopyable&) = delete;
	noncopyable(noncopyable&&) noexcept = default;

	noncopyable& operator=(const noncopyable&) = delete;
	noncopyable& operator=(noncopyable&&) noexcept = default;
};

} // namespace aw

#endif // aw_utility_variant_wrapper_h
