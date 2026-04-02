<template>
  <div class="settings">
    <h2>系统设置</h2>

    <el-tabs v-model="activeTab">
      <el-tab-pane label="个人信息" name="profile">
        <el-card>
          <el-form label-width="100px">
            <el-form-item label="用户名">
              <el-input :value="user?.username" disabled />
            </el-form-item>
            <el-form-item label="邮箱">
              <el-input :value="user?.email" disabled />
            </el-form-item>
            <el-form-item label="角色">
              <el-tag>{{ user?.role }}</el-tag>
            </el-form-item>
          </el-form>
        </el-card>
      </el-tab-pane>

      <el-tab-pane label="修改密码" name="password">
        <el-card>
          <el-form :model="pwdForm" label-width="100px">
            <el-form-item label="当前密码">
              <el-input v-model="pwdForm.old" type="password" />
            </el-form-item>
            <el-form-item label="新密码">
              <el-input v-model="pwdForm.new" type="password" />
            </el-form-item>
            <el-form-item label="确认密码">
              <el-input v-model="pwdForm.confirm" type="password" />
            </el-form-item>
            <el-form-item>
              <el-button type="primary" @click="changePassword">修改密码</el-button>
            </el-form-item>
          </el-form>
        </el-card>
      </el-tab-pane>

      <el-tab-pane label="关于" name="about">
        <el-card>
          <el-descriptions :column="1" border>
            <el-descriptions-item label="平台名称">天枢 TianShu</el-descriptions-item>
            <el-descriptions-item label="版本">1.0.0</el-descriptions-item>
            <el-descriptions-item label="描述">IoT 开发管理平台</el-descriptions-item>
          </el-descriptions>
        </el-card>
      </el-tab-pane>
    </el-tabs>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import api from '@/api'

const activeTab = ref('profile')
const user = ref(null)

const pwdForm = reactive({
  old: '',
  new: '',
  confirm: ''
})

const loadUser = async () => {
  const res = await api.auth.me()
  if (res.code === 0) {
    user.value = res.data
  }
}

const changePassword = () => {
  if (pwdForm.new !== pwdForm.confirm) {
    ElMessage.error('两次密码不一致')
    return
  }
  ElMessage.info('密码修改功能开发中')
}

onMounted(loadUser)
</script>

<style scoped>
.settings h2 {
  margin: 0 0 20px;
}
</style>
