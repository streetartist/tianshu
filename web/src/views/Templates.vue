<template>
  <div class="templates">
    <div class="header">
      <h2>设备类型管理</h2>
      <el-button type="primary" @click="showCreateDialog = true">
        <el-icon><Plus /></el-icon> 新建类型
      </el-button>
    </div>

    <el-row :gutter="20">
      <el-col :span="8" v-for="t in templates" :key="t.id">
        <el-card class="template-card" shadow="hover">
          <template #header>
            <div class="card-header">
              <span class="name">{{ t.display_name }}</span>
            </div>
          </template>
          <p class="desc">{{ t.description || '暂无描述' }}</p>
          <div class="meta">
            <span>数据点: {{ t.data_points?.length || 0 }}</span>
            <span>命令: {{ t.commands?.length || 0 }}</span>
          </div>
          <div class="actions">
            <el-button link type="primary" @click="viewTemplate(t)">查看</el-button>
            <el-button link type="primary" @click="editTemplate(t)">编辑</el-button>
            <el-button link type="danger" @click="deleteTemplate(t)">删除</el-button>
          </div>
        </el-card>
      </el-col>
    </el-row>

    <!-- 创建对话框 -->
    <el-dialog v-model="showCreateDialog" title="新建设备类型" width="600">
      <el-form :model="form" label-width="100px">
        <el-form-item label="类型标识" required>
          <el-input v-model="form.name" placeholder="小写字母和下划线，如 my_sensor" />
        </el-form-item>
        <el-form-item label="显示名称" required>
          <el-input v-model="form.display_name" placeholder="如：温湿度传感器" />
        </el-form-item>
        <el-form-item label="描述">
          <el-input v-model="form.description" type="textarea" rows="2" />
        </el-form-item>
        <el-form-item label="硬件平台">
          <el-select v-model="form.hardware_platform" style="width: 100%">
            <el-option label="ESP32" value="esp32" />
            <el-option label="ESP8266" value="esp8266" />
            <el-option label="Arduino" value="arduino" />
            <el-option label="其他" value="other" />
          </el-select>
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="showCreateDialog = false">取消</el-button>
        <el-button type="primary" @click="createTemplate">创建</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage, ElMessageBox } from 'element-plus'
import api from '@/api'

const router = useRouter()
const templates = ref([])
const showCreateDialog = ref(false)

const form = reactive({
  name: '',
  display_name: '',
  description: '',
  hardware_platform: 'esp32'
})

const loadTemplates = async () => {
  const res = await api.templates.list()
  templates.value = res.data || []
}

const createTemplate = async () => {
  if (!form.name || !form.display_name) {
    ElMessage.warning('请填写必填项')
    return
  }
  const res = await api.templates.create(form)
  if (res.code === 0) {
    ElMessage.success('创建成功')
    showCreateDialog.value = false
    router.push(`/templates/${res.data.id}`)
  }
}

const viewTemplate = (t) => {
  router.push(`/templates/${t.id}`)
}

const editTemplate = (t) => {
  router.push(`/templates/${t.id}?edit=1`)
}

const deleteTemplate = async (t) => {
  await ElMessageBox.confirm('确定删除该设备类型？', '提示', { type: 'warning' })
  const res = await api.templates.delete(t.id)
  if (res.code === 0) {
    ElMessage.success('已删除')
    loadTemplates()
  }
}

onMounted(loadTemplates)
</script>

<style scoped>
.header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
}
.header h2 { margin: 0; }
.template-card { margin-bottom: 20px; }
.card-header {
  display: flex;
  align-items: center;
  gap: 8px;
}
.card-header .name { font-weight: bold; }
.desc {
  color: #666;
  font-size: 14px;
  margin: 0 0 10px;
}
.meta {
  display: flex;
  gap: 16px;
  font-size: 12px;
  color: #999;
  margin-bottom: 10px;
}
.actions {
  border-top: 1px solid #eee;
  padding-top: 10px;
}
</style>
