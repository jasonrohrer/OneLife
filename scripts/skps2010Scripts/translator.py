#!/usr/bin/env python3
import requests
import os
from os.path import dirname
import sys


def main():
    print('Connect to Google sheet...\n')

    if getattr(sys, 'frozen', False):
        path = dirname(sys.executable)
    else:
        path = os.path.dirname(__file__)
    os.chdir(path)
    url = 'https://script.google.com/macros/s/AKfycbx0agAIW99KUpLdLQX1ghFaMu81uopoQ7zNqHe7s3D5gWIZO8cb7tLRTGV8Gb8F4saC/exec'

    try:
        r = requests.get(f'{url}')
    except requests.ConnectionError:
        print('Unable to connect to the Google sheet')
        return

    if r.status_code != 200:
        print('Unable to connect to the Google sheet')
        return

    langs = r.json()
    for i in range(len(langs)):
        print(f'{i}: {langs[i]}')
    print(f'Please input 0~{len(langs)-1}: ')
    while 1:
        try:
            lang = int(input())
            if lang < 0 or lang > len(langs) - 1:
                raise ValueError
            break
        except ValueError:
            print(f'Please input 0~{len(langs)-1}: ')

    print('\nAppend English after translated objects?')
    print('0: No')
    print('1: Yes')
    print('Please input 0~1: ')
    while 1:
        try:
            is_append = int(input())
            if is_append < 0 or is_append > 1:
                raise ValueError
            break
        except ValueError:
            print('Please input 0~1: ')

    print("Translating Objects...")

    if os.path.isfile('objects/cache.fcz'):
        os.remove('objects/cache.fcz')

    r = requests.get(f'{url}?lang={lang}&type=0')

    if is_append:
        r2 = requests.get(f'{url}?lang=0&type=0')
        data2 = r2.json()

    data = r.json()
    for i in range(len(data['key'])):
        translated = data['value'][i]
        if translated != '':
            try:
                with open(f'objects/{data["key"][i]}', encoding='utf-8') as f:
                    content = f.readlines()
            except FileNotFoundError as e:
                print(e)
                continue

            if is_append and data2['value'][i] != '':
                content[1] = translated.split('#')[0].split(
                    ' $')[0] + data2['value'][i] + '\n'
            else:
                content[1] = translated + '\n'

            with open(f'objects/{data["key"][i]}', 'w', encoding='utf-8') as f:
                f.writelines(content)

    menuItems = {}
    try:
        with open('languages/English.txt', encoding='utf-8') as f:
            datas = f.readlines()
            for data in datas:
                if data == '\n':
                    continue
                name = data.split(' ')[0]
                value = data[data.index('"') + 1:-2]
                menuItems[name] = value

    except FileNotFoundError as e:
        print(e)

    print("Translating Menu...")

    r = requests.get(f'{url}?lang={lang}&type=1')
    data = r.json()

    for i in range(len(data['key'])):
        if data['value'][i] != '':
            menuItems[data['key'][i]] = data['value'][i]

    with open('languages/English.txt', 'w', encoding='utf-8') as f:
        for key in menuItems:
            f.write(f'{key} "{menuItems[key]}"\n')

    print("Translating Images...")

    r = requests.get(f'{url}?lang={lang}&type=2')
    data = r.json()
    for i in range(len(data['key'])):
        link = data['value'][i]
        if link != '':
            r = requests.get(link)
            if r.status_code != 200:
                print(f'File can\'t be found: {link}')
                continue
            with open(f'graphics/{data["key"][i]}', 'wb') as f:
                f.write(r.content)

    print("Apply settings...")

    r = requests.get(f'{url}?lang={lang}&type=3')
    data = r.json()
    for i in range(len(data['key'])):
        value = data['value'][i]
        if value != '':
            with open(f'settings/{data["key"][i]}', 'w') as f:
                f.write(str(value))

    print("Translating done!")


if __name__ == '__main__':
    main()
