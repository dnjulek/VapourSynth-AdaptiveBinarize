#include "VapourSynth4.h"
#include "VSHelper4.h"

typedef struct AdaptiveBinarizeData {
	VSNode* node;
	VSNode* node2;
	const VSVideoInfo* vi;
	int c_param;
	int tab[768]{};
} AdaptiveBinarizeData;

static const VSFrame* VS_CC adaptiveBinarizeGetFrame(int n, int activationReason, void* instanceData, void** frameData, VSFrameContext* frameCtx, VSCore* core, const VSAPI* vsapi) {
	auto* d = reinterpret_cast<AdaptiveBinarizeData*>(instanceData);

	if (activationReason == arInitial) {
		vsapi->requestFrameFilter(n, d->node, frameCtx);
		vsapi->requestFrameFilter(n, d->node2, frameCtx);
	}
	else if (activationReason == arAllFramesReady) {
		const VSFrame* src = vsapi->getFrameFilter(n, d->node, frameCtx);
		const VSFrame* src2 = vsapi->getFrameFilter(n, d->node2, frameCtx);

		const VSVideoFormat* fi = vsapi->getVideoFrameFormat(src);
		int height = vsapi->getFrameHeight(src, 0);
		int width = vsapi->getFrameWidth(src, 0);

		VSFrame* dst = vsapi->newVideoFrame(fi, width, height, src, core);

		for (int plane = 0; plane < fi->numPlanes; plane++) {

			const uint8_t* srcp = vsapi->getReadPtr(src, plane);
			const uint8_t* src2p = vsapi->getReadPtr(src2, plane);
			uint8_t* dstp = vsapi->getWritePtr(dst, plane);

			ptrdiff_t stride = vsapi->getStride(src, plane);

			int h = vsapi->getFrameHeight(src, plane);
			int w = vsapi->getFrameWidth(src, plane);

			for (int y = 0; y < h; y++) {
				for (int x = 0; x < w; x++) {
					int z = (srcp[x] - src2p[x] + 255);
					dstp[x] = d->tab[z];
				}

				dstp += stride;
				srcp += stride;
				src2p += stride;
			}
		}

		vsapi->freeFrame(src);
		vsapi->freeFrame(src2);

		return dst;
	}

	return NULL;
}

static void VS_CC adaptiveBinarizeFree(void* instanceData, VSCore* core, const VSAPI* vsapi) {
	AdaptiveBinarizeData* d = (AdaptiveBinarizeData*)instanceData;
	vsapi->freeNode(d->node);
	vsapi->freeNode(d->node2);
	free(d);
}

static void VS_CC adaptiveBinarizeCreate(const VSMap* in, VSMap* out, void* userData, VSCore* core, const VSAPI* vsapi) {
	AdaptiveBinarizeData d;
	AdaptiveBinarizeData* data;
	int err;

	d.node = vsapi->mapGetNode(in, "clip", 0, 0);
	d.node2 = vsapi->mapGetNode(in, "clip2", 0, 0);
	d.vi = vsapi->getVideoInfo(d.node);

	if (!vsh::isConstantVideoFormat(d.vi) || d.vi->format.sampleType != stInteger || d.vi->format.bitsPerSample != 8) {
		vsapi->mapSetError(out, "AdaptiveBinarize: only constant format 8bit integer input supported");
		vsapi->freeNode(d.node);
		return;
	}

	if (!vsh::isSameVideoInfo(vsapi->getVideoInfo(d.node2), d.vi)) {
		vsapi->mapSetError(out, "AdaptiveBinarize: both clips must have the same format and dimensions");
		vsapi->freeNode(d.node);
		vsapi->freeNode(d.node2);
		return;
	}

	if (vsapi->getVideoInfo(d.node2)->numFrames != d.vi->numFrames) {
		vsapi->mapSetError(out, "AdaptiveBinarize: both clips' number of frames do not match");
		vsapi->freeNode(d.node);
		vsapi->freeNode(d.node2);
		return;
	}

	d.c_param = vsapi->mapGetIntSaturated(in, "c", 0, &err);
	if (err)
		d.c_param = 3;

	for (int i = 0; i < 768; i++) {
		d.tab[i] = i - 255 <= -d.c_param ? 255 : 0;
	}

	data = (AdaptiveBinarizeData*)malloc(sizeof(d));
	*data = d;

	VSFilterDependency deps[]{ {d.node, rpGeneral}, {d.node2, rpGeneral} };
	vsapi->createVideoFilter(out, "AdaptiveBinarize", data->vi, adaptiveBinarizeGetFrame, adaptiveBinarizeFree, fmParallel, deps, 2, data, core);
}

VS_EXTERNAL_API(void) VapourSynthPluginInit2(VSPlugin* plugin, const VSPLUGINAPI* vspapi) {
	vspapi->configPlugin("com.julek.abrz", "abrz", "Adaptive Binarize", VS_MAKE_VERSION(1, 0), VAPOURSYNTH_API_VERSION, 0, plugin);
	vspapi->registerFunction("AdaptiveBinarize",
		"clip:vnode;"
		"clip2:vnode;"
		"c:int:opt;",
		"clip:vnode;",
		adaptiveBinarizeCreate, NULL, plugin);
}