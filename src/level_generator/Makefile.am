NULL=

bin_PROGRAMS=level_generator

level_generator_onedir=$(libdir)

level_generator_SOURCES=						\
	level_generator.cpp							\
	level_generator.hpp							\
	GeneratorBase.cpp							\
	GeneratorBase.hpp							\
	BruteForceGenerator.cpp						\
	BruteForceGenerator.hpp						\
	RandGenerators.cpp							\
	RandGenerators.hpp							\
	QuadTree.cpp								\
	QuadTree.hpp								\
	OcclusionBuffer.cpp							\
	OcclusionBuffer.hpp							\
	Utils.cpp									\
	Utils.hpp									\
	$(NULL)

AM_CPPFLAGS=									\
	$(LEVEL_GENERATOR_DEPS_CFLAGS)				\
	$(BOOST_CPPFLAGS)							\
	$(BOOST_PROGRAM_OPTIONS_CPPFLAGS)			\
	$(BOOST_DATE_TIME_CPPFLAGS)					\
	$(BOOST_THREAD_CPPFLAGS)					\
	$(NULL)

level_generator_LDADD=							\
	$(LEVEL_GENERATOR_DEPS_LIBS)				\
	$(BOOST_LIBS)								\
	$(BOOST_PROGRAM_OPTIONS_LIBS)				\
	$(BOOST_DATE_TIME_LIBS)						\
	$(BOOST_THREAD_LIBS)						\
	$(NULL)

CLEANFILES=										\
	.libs										\
	$(NULL)
