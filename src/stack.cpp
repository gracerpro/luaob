// LocalVarsStack.cpp

#include "Stack.h"

LocalVarsStack::LocalVarsStack(size_t initCount) {
	if (initCount < STACK_CAPACITY)
		initCount = STACK_CAPACITY;
	else if (initCount % STACK_CAPACITY != 0)
		initCount = initCount / STACK_CAPACITY + 1;

	m_data = new stObfuscatedName[initCount];
	m_count = 0;
	m_reservedCount = initCount;
}

LocalVarsStack::~LocalVarsStack() {
	if (m_data)
		delete[] m_data;
}

void LocalVarsStack::push(const stObfuscatedName& obfuscatedName) {
	if (m_count == m_reservedCount) {
		m_reservedCount += STACK_CAPACITY;
		stObfuscatedName *pNewData = new stObfuscatedName[m_reservedCount];
		for (size_t i = 0; i < m_count; ++i)
			pNewData[i] = m_data[i];
		delete[] m_data;
		m_data = pNewData;
	}

	m_data[m_count] = obfuscatedName;
	++m_count;
}

void LocalVarsStack::push(const char *name, const char *fake_name) {
	if (m_count == m_reservedCount) {
		m_reservedCount += STACK_CAPACITY;
		stObfuscatedName *pNewData = new stObfuscatedName[m_reservedCount];
		for (size_t i = 0; i < m_count; ++i)
			pNewData[i] = m_data[i];
		delete[] m_data;
	}

	m_data[m_count].name = name;
	m_data[m_count].fake_name = fake_name;
	++m_count;
}

void LocalVarsStack::pop() {
	if (!m_count)
		return;

	--m_count;
}

void LocalVarsStack::pops(size_t elementCount) {
	if (elementCount > m_count)
		m_count = 0;
	else
		m_count -= elementCount;
}

LocalVarsStack& operator+= (LocalVarsStack& stackDest, const LocalVarsStack& stackSource) {
	int top = stackSource.getTopIndex();
	int i = 0;

	while (i <= top) {
		stackDest.push(stackSource.items(i));
		++i;
	}

	return stackDest;
}

bool LocalVarsStack::find(const std::string& name) const {
	for (size_t i = 0; i < m_count; ++i) {
		if (m_data[i].name == name)
			return true;
	}

	return false;
}

bool LocalVarsStack::find(const char *name) const {
	for (size_t i = 0; i < m_count; ++i) {
		if (m_data[i].name == name)
			return true;
	}

	return false;
}

bool LocalVarsStack::find(stObfuscatedName& obfuscatedName) const {
	if (!m_count)
		return false;

	int top = m_count - 1;
	while (top >= 0) {
		if (m_data[top].name == obfuscatedName.name) {
			obfuscatedName.fake_name = m_data[top].fake_name;
			return true;
		}
		--top;
	}

	return false;
}

stObfuscatedName& LocalVarsStack::top() const {
	if (!m_count)
		return m_data[0];

	return m_data[m_count - 1];
}

int LocalVarsStack::getTopIndex() const {
	return m_count - 1;
}

stObfuscatedName& LocalVarsStack::items(const size_t index) const {
	return m_data[index];
}

size_t LocalVarsStack::count() const {
	return m_count;
}

bool LocalVarsStack::empty() const {
	return m_count == 0;
}