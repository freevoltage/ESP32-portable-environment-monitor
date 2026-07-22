// @ts-check
import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';

// https://astro.build/config
export default defineConfig({
	site: 'https://freevoltage.github.io',
	base: '/ESP32-portable-environment-monitor',
	integrations: [
		starlight({
			title: 'ESP32 Hiking Weather Station',
			social: [
				{ icon: 'github', label: 'GitHub', href: 'https://github.com/freevoltage/ESP32-portable-environment-monitor' },
			],
			sidebar: [
				{
					label: 'Getting Started',
					items: [
						{ label: 'Overview', slug: 'index' },
						{ label: 'Hardware Setup', slug: 'getting-started/hardware-setup' },
						{ label: 'Firmware Install', slug: 'getting-started/firmware-install' },
						{ label: 'First Run', slug: 'getting-started/first-run' },
					],
				},
				{
					label: 'User Interface',
					items: [
						{ label: 'Screens & Navigation', slug: 'user-interface/index' },
					],
				},
				{
					label: 'Architecture',
					items: [
						{ label: 'System Overview', slug: 'architecture/overview' },
						{ label: 'Hardware Layer', slug: 'architecture/hardware-layer' },
						{ label: 'Services Layer', slug: 'architecture/services-layer' },
					],
				},
				{
					label: 'Configuration',
					items: [
						{ label: 'config.h Reference', slug: 'configuration/config-h' },
						{ label: 'Deep Sleep & Power', slug: 'configuration/deep-sleep' },
					],
				},
				{
					label: 'Development',
					items: [
						{ label: 'Testing', slug: 'development/testing' },
						{ label: 'Examples', slug: 'development/examples' },
						{ label: 'Adding Features', slug: 'development/adding-features' },
						{ label: 'Wiki Development', slug: 'development/wiki-dev' },
					],
				},
				{
					label: 'Reference',
					items: [{ autogenerate: { directory: 'reference' } }],
				},
			],
		}),
	],
});
