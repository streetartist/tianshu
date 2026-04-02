import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../../../core/constants/app_constants.dart';
import '../../widgets/widget_type.dart';
import '../state/canvas_notifier.dart';
import 'canvas_widget_item.dart';

/// 画布区域 - 中间手机模拟器预览
class CanvasArea extends ConsumerWidget {
  const CanvasArea({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    return Container(
      color: Colors.grey.shade200,
      child: Center(
        child: _buildPhoneFrame(context, ref),
      ),
    );
  }

  Widget _buildPhoneFrame(BuildContext context, WidgetRef ref) {
    return Container(
      width: AppConstants.phonePreviewWidth + 20,
      height: AppConstants.phonePreviewHeight + 40,
      decoration: BoxDecoration(
        color: Colors.black,
        borderRadius: BorderRadius.circular(40),
      ),
      padding: const EdgeInsets.all(10),
      child: ClipRRect(
        borderRadius: BorderRadius.circular(30),
        child: _buildCanvas(context, ref),
      ),
    );
  }

  Widget _buildCanvas(BuildContext context, WidgetRef ref) {
    final canvasState = ref.watch(canvasProvider);

    return DragTarget<WidgetType>(
      onAcceptWithDetails: (details) {
        final RenderBox box = context.findRenderObject() as RenderBox;
        final localPos = box.globalToLocal(details.offset);
        ref.read(canvasProvider.notifier).addWidget(
              details.data,
              localPos.dx.clamp(0, AppConstants.phonePreviewWidth - 50),
              localPos.dy.clamp(0, AppConstants.phonePreviewHeight - 30),
            );
      },
      builder: (context, candidateData, rejectedData) {
        return GestureDetector(
          onTap: () => ref.read(canvasProvider.notifier).selectWidget(null),
          child: Container(
            color: candidateData.isNotEmpty
                ? Colors.blue.shade50
                : Colors.white,
            child: Stack(
              children: [
                if (canvasState.widgets.isEmpty)
                  const Center(
                    child: Text('拖拽控件到此处',
                        style: TextStyle(color: Colors.grey)),
                  ),
                ...canvasState.widgets.map((w) => CanvasWidgetItem(
                      widget: w,
                      isSelected: w.id == canvasState.selectedWidgetId,
                    )),
              ],
            ),
          ),
        );
      },
    );
  }
}
