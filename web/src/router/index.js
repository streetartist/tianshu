import { createRouter, createWebHistory } from 'vue-router'

const routes = [
  {
    path: '/login',
    name: 'Login',
    component: () => import('@/views/Login.vue')
  },
  {
    path: '/',
    component: () => import('@/layouts/MainLayout.vue'),
    children: [
      {
        path: '',
        name: 'Dashboard',
        component: () => import('@/views/Dashboard.vue')
      },
      {
        path: 'devices',
        name: 'Devices',
        component: () => import('@/views/Devices.vue')
      },
      {
        path: 'devices/:id',
        name: 'DeviceDetail',
        component: () => import('@/views/DeviceDetail.vue')
      },
      {
        path: 'gateway',
        name: 'Gateway',
        component: () => import('@/views/Gateway.vue')
      },
      {
        path: 'agents',
        name: 'Agents',
        component: () => import('@/views/Agents.vue')
      },
      {
        path: 'agents/:id/chat',
        name: 'AgentChat',
        component: () => import('@/views/AgentChat.vue')
      },
      {
        path: 'data',
        name: 'DataView',
        component: () => import('@/views/DataView.vue')
      },
      {
        path: 'settings',
        name: 'Settings',
        component: () => import('@/views/Settings.vue')
      },
      {
        path: 'logs',
        name: 'Logs',
        component: () => import('@/views/Logs.vue')
      },
      {
        path: 'templates',
        name: 'Templates',
        component: () => import('@/views/Templates.vue')
      },
      {
        path: 'templates/:id',
        name: 'TemplateDetail',
        component: () => import('@/views/TemplateDetail.vue')
      }
    ]
  }
]

const router = createRouter({
  history: createWebHistory(),
  routes
})

// 路由守卫
router.beforeEach((to, from, next) => {
  const token = localStorage.getItem('token')
  if (to.path !== '/login' && !token) {
    next('/login')
  } else {
    next()
  }
})

export default router
