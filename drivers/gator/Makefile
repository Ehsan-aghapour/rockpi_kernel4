

CONFIG_GATOR ?= m
obj-$(CONFIG_GATOR) := gator.o

gator-y :=  gator_main.o \
            gator_events_block.o \
            gator_events_irq.o \
            gator_events_meminfo.o \
            gator_events_mmapped.o \
            gator_events_net.o \
            gator_events_sched.o

       chk_events.h = :
quiet_chk_events.h = echo '  CHK     $@'
silent_chk_events.h = :



CONFIG_GATOR_MALI_4XXMP := n
CONFIG_GATOR_MALI_MIDGARD := y
EXTRA_CFLAGS += -DMALI_SUPPORT=MALI_MIDGARD_OR_BIFROST
CONFIG_GATOR_MALI_MIDGARD_PATH ?= drivers/gpu/arm/bifrost-hikey970
gator-y +=  gator_events_mali_midgard.o \
     	gator_events_mali_midgard_hw.o
include $(src)/mali_midgard.mk
gator-y +=    gator_events_mali_common.o

ccflags-$(CONFIG_GATOR_MALI_MIDGARD) += -I$(CONFIG_GATOR_MALI_MIDGARD_PATH)
ccflags-$(CONFIG_GATOR_MALI_MIDGARD) += -DMALI_SUPPORT=MALI_MIDGARD_OR_BIFROST




gator-additional-dependencies := $(obj)/generated_gator_src_md5.h
clean-files := generated_gator_src_md5.h

# Should the original or new block_rq_complete API be used?
OLD_BLOCK_RQ_COMPLETE := $(shell grep -A3 block_rq_complete $(srctree)/include/trace/events/block.h | grep nr_bytes -q; echo $$?)
EXTRA_CFLAGS += -DOLD_BLOCK_RQ_COMPLETE=$(OLD_BLOCK_RQ_COMPLETE)

gator-$(CONFIG_ARM) +=  gator_events_l2c-310.o

gator-$(CONFIG_ARM64) +=

$(obj)/gator_main.o: $(gator-additional-dependencies)

# Note, in the recipe below we use "cd $(srctree) && cd $(src)" rather than
# "cd $(srctree)/$(src)" because under DKMS $(src) is an absolute path, and we
# can't just use $(src) because for normal kernel builds this is relative to
# $(srctree)
$(obj)/generated_gator_src_md5.h: $(wildcard $(src)/gator_*.h $(src)/gator_*.c $(src)/mali/*.h)
	@$($(quiet)chk_events.h)
	$(Q)cd $(srctree) && cd $(src) ; $(CONFIG_SHELL) -c "echo 'static char * gator_src_md5 = \"'\`ls gator_*.c gator_*.h mali/*.h | grep -Ev '^(generated_gator_src_md5\.h|gator\.mod\.c)$$' | LC_ALL=C sort | xargs cat | md5sum | cut -b 1-32\`'\";'" > $(abspath $@)



