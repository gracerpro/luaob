// stack.h
#include <string>

struct stObfuscatedName
{
	std::string name;
	std::string fake_name;
};

class LocalVarsStack {
public:
	LocalVarsStack(size_t initCount = STACK_CAPACITY);
	~LocalVarsStack();

	void push(const stObfuscatedName& obfuscatedName);
	void push(const char *name, const char *fake_name);
	void pop();
	void pops(size_t elementCount = 1);

	friend LocalVarsStack& operator+= (LocalVarsStack& stackDest, const LocalVarsStack& stackSource);

	stObfuscatedName& top() const;
	int getTopIndex() const;
	
	stObfuscatedName& items(const size_t index) const;
	
	bool find(stObfuscatedName& obfuscatedName) const;
	bool find(const std::string& name) const;
	
	size_t count() const;
	bool empty() const;

private:
	enum {
		STACK_CAPACITY = 16,
		INDEX_NULL     = -1
	};

	stObfuscatedName* m_data;
	size_t m_count;
	size_t m_reservedCount;
};
