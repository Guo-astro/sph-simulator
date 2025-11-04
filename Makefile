CXX = g++

# C++11
CXXFLAGS = -std=gnu++14

# Optimization
CXXFLAGS += -O3 -funroll-loops -ffast-math

# OpenMP
CXXFLAGS += -fopenmp

# use for debug
CXXFLAGS += -Wall
CXXFLAGS += -Wno-sign-compare
CXXFLAGS += -Wno-maybe-uninitialized
#CXXFLAGS += -g =D_DEBUG

SRCDIR = src
BUILDDIR = obj
INC = -Iinclude

SRCEXT = cpp
DEPEXT = d
OBJEXT = o

TARGET = sph

# Makefileの参考: https://minus9d.hatenablog.com/entry/2017/10/20/222901
# DO NOT EDIT BELOW THIS LINE
#-------------------------------------------------------

sources = $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
objects = $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(subst $(SRCEXT),$(OBJEXT),$(sources)))
dependencies = $(subst .$(OBJEXT),.$(DEPEXT),$(objects))

# Defauilt Make
all: $(TARGET)

# Remake
remake: clean all

# ディレクトリ生成
directories:
	@mkdir -p $(BUILDDIR)

# 中間生成物のためのディレクトリを削除
clean:
	@$(RM) -rf $(BUILDDIR)/* $(TARGET)

# 自動抽出した.dファイルを読み込む
-include $(dependencies)

# オブジェクトファイルをリンクしてバイナリを生成
$(TARGET): $(objects)
	$(CXX) -o $(TARGET) $(CXXFLAGS) $^ $(FLAGS)

# ソースファイルのコンパイルしてオブジェクトファイルを生成
# また、ソースファイルの依存関係を自動抽出して.dファイルに保存
$(BUILDDIR)/%.$(OBJEXT): $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INC) -c -o $@ $<
	@$(CXX) $(CXXFLAGS) $(INC) -MM $(SRCDIR)/$*.$(SRCEXT) > $(BUILDDIR)/$*.$(DEPEXT)
	@cp -f $(BUILDDIR)/$*.$(DEPEXT) $(BUILDDIR)/$*.$(DEPEXT).tmp
	@sed -e 's|.*:|$(BUILDDIR)/$*.$(OBJEXT):|' < $(BUILDDIR)/$*.$(DEPEXT).tmp > $(BUILDDIR)/$*.$(DEPEXT)
	@sed -e 's/.*://' -e 's/\\$$//' < $(BUILDDIR)/$*.$(DEPEXT).tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $(BUILDDIR)/$*.$(DEPEXT)
	@rm -f $(BUILDDIR)/$*.$(DEPEXT).tmp

# Non-File Targets
.PHONY: all remake clean format format-check lint tidy quality-check

# Code Quality Targets
# ---------------------

# Format all C++ source files
format:
	@echo "Formatting C++ files..."
	@find include src workflows -name '*.cpp' -o -name '*.hpp' -o -name '*.tpp' | xargs clang-format -i
	@echo "✅ Formatting complete"

# Check formatting without modifying files
format-check:
	@echo "Checking code formatting..."
	@FAILED=0; \
	for file in $$(find include src workflows -name '*.cpp' -o -name '*.hpp' -o -name '*.tpp'); do \
		if ! clang-format --dry-run --Werror "$$file" 2>/dev/null; then \
			echo "❌ Format check failed: $$file"; \
			FAILED=1; \
		fi; \
	done; \
	if [ $$FAILED -eq 0 ]; then \
		echo "✅ All files properly formatted"; \
	else \
		echo "Run 'make format' to fix formatting issues"; \
		exit 1; \
	fi

# Run clang-tidy on source files
tidy:
	@echo "Running clang-tidy..."
	@if [ ! -f build/compile_commands.json ]; then \
		echo "⚠️  Compilation database not found. Run: cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .."; \
	fi
	@find include src -name '*.cpp' -o -name '*.hpp' | head -20 | xargs clang-tidy -p build 2>&1 | grep -v "warnings generated"
	@echo "✅ Clang-tidy check complete"

# Comprehensive quality check
quality-check: format-check
	@echo "Running comprehensive quality checks..."
	@echo "1. Checking for TODO/FIXME comments..."
	@grep -rn "TODO\|FIXME" include src || echo "  ✅ No TODO/FIXME found"
	@echo "2. Checking for common issues..."
	@grep -rn "cout\|printf" src include || echo "  ✅ No debug output found"
	@echo "✅ Quality check complete"
