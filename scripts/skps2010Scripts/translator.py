#!/usr/bin/env python3
import requests
import os


def main():
    print('0: English')
    print('1: 正體中文')
    print('2: 简体中文')
    print('3: Українська')
    print('Please input 0~3: ')
    while 1:
        try:
            lang = int(input())
            if lang < 0 or lang > 3:
                raise ValueError
            break
        except ValueError:
            print('Please input 0~3: ')

    print("Translating...")

    if os.path.isfile('objects/cache.fcz'):
        os.remove('objects/cache.fcz')

    r = requests.get(
        f'https://script.google.com/macros/s/AKfycbx0agAIW99KUpLdLQX1ghFaMu81uopoQ7zNqHe7s3D5gWIZO8cb7tLRTGV8Gb8F4saC/exec?lang={lang}&type=0'
    )
    data = r.json()
    for i in range(len(data['key'])):
        if data['value'][i] != '':
            with open(f'objects/{data["key"][i]}', encoding='utf-8') as f:
                content = f.readlines()
            content[1] = data['value'][i] + '\n'
            with open(f'objects/{data["key"][i]}', 'w', encoding='utf-8') as f:
                f.writelines(content)

    menuItems = {}
    with open('languages/English.txt', encoding='utf-8') as f:
        datas = f.readlines()
        for data in datas:
            if data == '\n':
                continue
            name = data.split(' ')[0]
            value = data[data.index('"') + 1:-2]
            menuItems[name] = value

    r = requests.get(
        f'https://script.google.com/macros/s/AKfycbx0agAIW99KUpLdLQX1ghFaMu81uopoQ7zNqHe7s3D5gWIZO8cb7tLRTGV8Gb8F4saC/exec?lang={lang}&type=1'
    )
    data = r.json()

    for i in range(len(data['key'])):
        if data['value'][i] != '':
            menuItems[data['key'][i]] = data['value'][i]

    with open('languages/English.txt', 'w', encoding='utf-8') as f:
        for key in menuItems:
            f.write(f'{key} "{menuItems[key]}"\n')

    print("Translating done!")


if __name__ == '__main__':
    main()
