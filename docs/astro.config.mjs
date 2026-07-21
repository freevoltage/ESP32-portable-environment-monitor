// @ts-check
import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';

// https://astro.build/config
export default defineConfig({
	site: 'https://davidwitulla.github.io',
	integrations: [
		starlight({
			title: 'ESP32 Hiking Weather Station',
			social: [
				{ icon: 'github', label: 'GitHub', href: 'https://github.com/davidwitulla/ESP32_BME280' },
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
