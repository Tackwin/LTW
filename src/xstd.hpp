namespace details {
	template<typename Callable>
	struct Defer {
		~Defer() noexcept { todo(); }
		Defer(Callable todo) noexcept : todo(todo) {};
	private:
		Callable todo;
	};
};

#define CONCAT_(x, y) x##y
#define CONCAT(x, y) CONCAT_(x, y)
#define defer details::Defer CONCAT(defer_, __COUNTER__) = [&]

#define BEG(x) std::begin(x)
#define END(x) std::end(x)
#define BEG_END(x) BEG(x), END(x)
#define V2F(x) Vector2f{(x), (x)}
