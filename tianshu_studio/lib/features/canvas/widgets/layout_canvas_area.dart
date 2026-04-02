import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../../../core/constants/app_constants.dart';
import '../../../data/models/layout_models.dart';
import '../../widgets/widget_type.dart';
import '../state/layout_canvas_notifier.dart';
import 'layout_item_widget.dart';

/// 布局画布区域
class LayoutCanvasArea extends ConsumerWidget {
  const LayoutCanvasArea({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    return Container(
      color: Colors.grey.shade200,
      child: Center(child: _buildPhoneFrame(context, ref)),
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
    final canvasState = ref.watch(layoutCanvasProvider);
    final page = canvasState.page;

    return DragTarget<Object>(
      onWillAcceptWithDetails: (details) {
        return details.data is WidgetType || details.data is LayoutItem;
      },
      onAcceptWithDetails: (details) {
        final notifier = ref.read(layoutCanvasProvider.notifier);
        if (details.data is WidgetType) {
          notifier.addItem((details.data as WidgetType).name);
        } else if (details.data is LayoutItem) {
          // 从容器拖出到页面根级别
          notifier.moveItemToRoot((details.data as LayoutItem).id);
        }
      },
      builder: (context, candidateData, rejectedData) {
        return Container(
          color: candidateData.isNotEmpty ? Colors.blue.shade50 : Colors.white,
          child: _buildLayout(page, canvasState.selectedItemId, ref),
        );
      },
    );
  }

  Widget _buildLayout(PageLayout page, String? selectedId, WidgetRef ref) {
    if (page.children.isEmpty) {
      return const Center(
        child: Text('拖拽控件到此处', style: TextStyle(color: Colors.grey)),
      );
    }

    return ReorderableListView.builder(
      padding: const EdgeInsets.all(8),
      itemCount: page.children.length,
      onReorder: (oldIndex, newIndex) {
        ref.read(layoutCanvasProvider.notifier).reorderItem(oldIndex, newIndex);
      },
      proxyDecorator: (child, index, animation) {
        return Material(
          elevation: 4,
          borderRadius: BorderRadius.circular(8),
          child: child,
        );
      },
      itemBuilder: (context, index) {
        final item = page.children[index];
        return LayoutItemWidget(
          key: ValueKey(item.id),
          item: item,
          isSelected: item.id == selectedId,
        );
      },
    );
  }
}
